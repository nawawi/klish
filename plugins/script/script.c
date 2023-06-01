/*
 *
 */

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <syslog.h>

#include <faux/str.h>
#include <faux/list.h>
#include <klish/kcontext.h>
#include <klish/ksession.h>


static char *script_mkfifo(void)
{
	char *name = NULL;

	name = faux_str_sprintf("/tmp/klish.fifo.%u.XXXXXX", getpid());
	mktemp(name);
	if (mkfifo(name, 0600) < 0) {
		faux_str_free(name);
		return NULL;
	}

	return name;
}


const char *kcontext_type_e_str[] = {
	"none",
	"plugin_init",
	"plugin_fini",
	"action",
	"service_action"
	};

#define PREFIX "KLISH_"
#define OVERWRITE 1


static bool_t populate_env_kpargv(const kpargv_t *pargv, const char *prefix)
{
	const kentry_t *cmd = NULL;
	faux_list_node_t *iter = NULL;
	faux_list_node_t *cur = NULL;

	if (!pargv)
		return BOOL_FALSE;

	// Command
	cmd = kpargv_command(pargv);
	if (cmd) {
		char *var = faux_str_sprintf("%sCOMMAND", prefix);
		setenv(var, kentry_name(cmd), OVERWRITE);
		faux_str_free(var);
	}

	// Parameters
	iter = faux_list_head(kpargv_pargs(pargv));
	while ((cur = faux_list_each_node(&iter))) {
		kparg_t *parg = (kparg_t *)faux_list_data(cur);
		kparg_t *tmp_parg = NULL;
		const kentry_t *entry = kparg_entry(parg);
		faux_list_node_t *iter_before = faux_list_prev_node(cur);
		faux_list_node_t *iter_after = cur;
		unsigned int num = 0;
		bool_t already_populated = BOOL_FALSE;

		if (!kparg_value(parg)) // PTYPE can contain parg with NULL value
			continue;

		// Search for such entry within arguments before current one
		while ((tmp_parg = (kparg_t *)faux_list_eachr(&iter_before))) {
			if (kparg_entry(tmp_parg) == entry) {
				already_populated = BOOL_TRUE;
				break;
			}
		}
		if (already_populated)
			continue;

		// Populate all next args with the current entry
		while ((tmp_parg = (kparg_t *)faux_list_each(&iter_after))) {
			char *var = NULL;
			const char *value = kparg_value(tmp_parg);
			if (kparg_entry(tmp_parg) != entry)
				continue;
			if (num == 0) {
				var = faux_str_sprintf("%sPARAM_%s",
					prefix, kentry_name(entry));
				setenv(var, value, OVERWRITE);
				faux_str_free(var);
			}
			var = faux_str_sprintf("%sPARAM_%s_%u",
				prefix, kentry_name(entry), num);
			setenv(var, value, OVERWRITE);
			faux_str_free(var);
			num++;
		}
	}

	return BOOL_TRUE;
}


static bool_t populate_env(kcontext_t *context)
{
	kcontext_type_e type = KCONTEXT_TYPE_NONE;
	const kentry_t *entry = NULL;
	const ksession_t *session = NULL;
	const char *str = NULL;
	pid_t pid = -1;
	uid_t uid = -1;

	assert(context);
	session = kcontext_session(context);
	assert(session);

	// Type
	type = kcontext_type(context);
	if (type >= KCONTEXT_TYPE_MAX)
		type = KCONTEXT_TYPE_NONE;
	setenv(PREFIX"TYPE", kcontext_type_e_str[type], OVERWRITE);

	// Candidate
	entry = kcontext_candidate_entry(context);
	if (entry)
		setenv(PREFIX"CANDIDATE", kentry_name(entry), OVERWRITE);

	// Value
	str = kcontext_candidate_value(context);
	if (str)
		setenv(PREFIX"VALUE", str, OVERWRITE);

	// PID
	pid = ksession_pid(session);
	if (pid != -1) {
		char *t = faux_str_sprintf("%lld", (long long int)pid);
		setenv(PREFIX"PID", t, OVERWRITE);
		faux_str_free(t);
	}

	// UID
	uid = ksession_uid(session);
	if (uid != -1) {
		char *t = faux_str_sprintf("%lld", (long long int)uid);
		setenv(PREFIX"UID", t, OVERWRITE);
		faux_str_free(t);
	}

	// User
	str = ksession_user(session);
	if (str)
		setenv(PREFIX"USER", str, OVERWRITE);

	// Parameters
	populate_env_kpargv(kcontext_pargv(context), PREFIX);

	// Parent parameters
	populate_env_kpargv(kcontext_parent_pargv(context), PREFIX"PARENT_");

	return BOOL_TRUE;
}


static char *find_out_shebang(const char *script)
{
	char *default_shebang = "/bin/sh";
	char *shebang = NULL;
	char *line = NULL;

	line = faux_str_getline(script, NULL);
	if (
		!line ||
		(strlen(line) < 2) ||
		(line[0] != '#') ||
		(line[1] != '!')
		) {
		faux_str_free(line);
		return faux_str_dup(default_shebang);
	}

	shebang = faux_str_dup(line + 2);
	faux_str_free(line);

	return shebang;
}


// Execute script
int script_script(kcontext_t *context)
{
	const char *script = NULL;
	pid_t cpid = -1;
	int res = -1;
	char *fifo_name = NULL;
	FILE *wpipe = NULL;
	char *command = NULL;
	char *shebang = NULL;

	script = kcontext_script(context);
	if (faux_str_is_empty(script))
		return 0;

	// Create FIFO
	if (!(fifo_name = script_mkfifo())) {
		fprintf(stderr, "Error: Can't create temporary FIFO.\n"
			"Error: The ACTION will be not executed.\n");
		return -1;
	}

	// Create process to write to FIFO
	cpid = fork();
	if (cpid == -1) {
		fprintf(stderr, "Error: Can't fork the write process.\n"
			"Error: The ACTION will be not executed.\n");
		unlink(fifo_name);
		faux_str_free(fifo_name);
		return -1;
	}

	// Child: write to FIFO
	if (cpid == 0) {
		wpipe = fopen(fifo_name, "w");
		faux_str_free(fifo_name);
		if (!wpipe)
			_exit(-1);
		fwrite(script, strlen(script), 1, wpipe);
		fflush(wpipe);
		fclose(wpipe);
		_exit(0);
	}

	// Parent
	// Populate environment. Put command parameters to env vars.
	populate_env(context);

	// Prepare command
	shebang = find_out_shebang(script);
	command = faux_str_sprintf("%s %s", shebang, fifo_name);

	res = system(command);

	// Wait for the writing process
	kill(cpid, SIGTERM);
	while (waitpid(cpid, NULL, 0) != cpid);

	// Clean up
	faux_str_free(command);
	unlink(fifo_name);
	faux_str_free(fifo_name);
	faux_str_free(shebang);

	return WEXITSTATUS(res);
}
