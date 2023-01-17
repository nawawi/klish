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
	const kentry_t *entry = NULL;
	kpargv_pargs_node_t *iter = NULL;
	kparg_t *parg = NULL;
	const kentry_t *saved_entry = NULL;
	size_t num = 0;

	if (!pargv)
		return BOOL_FALSE;

	// Command
	entry = kpargv_command(pargv);
	if (entry) {
		char *var = faux_str_sprintf("%sCOMMAND", prefix);
		setenv(var, kentry_name(entry), OVERWRITE);
		faux_str_free(var);
	}

	// Parameters
	iter = kpargv_pargs_iter(pargv);
	while ((parg = kpargv_pargs_each(&iter))) {
		char *var = NULL;
		entry = kparg_entry(parg);
		if (kentry_max(entry) > 1) { // Multi
			if (entry == saved_entry)
				num++;
			else
				num = 0;
			var = faux_str_sprintf("%sPARAM_%s_%u",
				prefix, kentry_name(entry), num);
			saved_entry = entry;
		} else { // Single
			var = faux_str_sprintf("%sPARAM_%s",
				prefix, kentry_name(entry));
			saved_entry = NULL;
			num = 0;
		}
		setenv(var, kparg_value(parg), OVERWRITE);
		faux_str_free(var);
	}

	return BOOL_TRUE;
}


static bool_t populate_env(kcontext_t *context)
{
	kcontext_type_e type = KCONTEXT_TYPE_NONE;
	const kentry_t *entry = NULL;
	const char *str = NULL;

	assert(context);

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
