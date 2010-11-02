/*
 * clish_script_callback.c
 *
 *
 * Callback hook to action a shell script.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "private.h"

#define KLISH_FIFO "/tmp/klish.fifo"

/*--------------------------------------------------------- */
bool_t clish_script_callback(clish_shell_t * this,
	const clish_command_t * cmd, const char *script)
{
	const char * shebang = NULL;
	pid_t cpid;
	char buf;
	int res;
	const char *fifo_name;
	FILE *rpipe, *wpipe;
	char *command = NULL;

	/* Signal vars */
	struct sigaction sig_old_int;
	struct sigaction sig_old_quit;
	struct sigaction sig_new;
	sigset_t sig_set;

	assert(this);
	assert(cmd);
	if (!script) /* Nothing to do */
		return BOOL_TRUE;

	/* Find out shebang */
	shebang = clish_command__get_shebang(cmd);
	if (!shebang)
		shebang = clish_shell__get_default_shebang(this);
	assert(shebang);

#ifdef DEBUG
	fprintf(stderr, "SHEBANG: #!%s\n", shebang);
	fprintf(stderr, "SCRIPT: %s\n", script);
#endif /* DEBUG */

	/* Get FIFO */
	fifo_name = clish_shell__get_fifo(this);
	if (!fifo_name) {
		fprintf(stderr, "System error. Can't create temporary FIFO.\n"
			"The ACTION will be not executed.\n");
		return BOOL_FALSE;
	}

	/* Create process to execute script */
	cpid = fork();
	if (cpid == -1) {
		fprintf(stderr, "System error. Can't fork the write process.\n"
			"The ACTION will be not executed.\n");
		return BOOL_FALSE;
	}

	/* Child */
	if (cpid == 0) {
		int retval;
		wpipe = fopen(fifo_name, "w");
		if (!wpipe)
			_exit(-1);
		fwrite(script, strlen(script) + 1, 1, wpipe);
		fclose(wpipe);
		_exit(0);
	}

	/* Parent */
	/* Ignore SIGINT and SIGQUIT */
	sigemptyset(&sig_set);
	sig_new.sa_flags = 0;
	sig_new.sa_mask = sig_set;
	sig_new.sa_handler = SIG_IGN;
	sigaction(SIGINT, &sig_new, &sig_old_int);
	sigaction(SIGQUIT, &sig_new, &sig_old_quit);

	/* Prepare command */
	lub_string_cat(&command, shebang);
	lub_string_cat(&command, " ");
	lub_string_cat(&command, fifo_name);

	/* Execute shebang with FIFO as argument */
	rpipe = popen(command, "r");
	lub_string_free(command);
	if (!rpipe) {
		fprintf(stderr, "System error. Can't fork the script.\n"
			"The ACTION will be not executed.\n");
		kill(cpid, SIGTERM);
		waitpid(cpid, NULL, 0);

		/* Restore SIGINT and SIGQUIT */
		sigaction(SIGINT, &sig_old_int, NULL);
		sigaction(SIGQUIT, &sig_old_quit, NULL);

		return BOOL_FALSE;
	}
	/* Read the result of script execution */
	while (read(fileno(rpipe), &buf, 1) > 0)
		write(fileno(clish_shell__get_ostream(this)), &buf, 1);
	/* Wait for the writing process */
	waitpid(cpid, NULL, 0);
	/* Wait for script */
	res = pclose(rpipe);

	/* Restore SIGINT and SIGQUIT */
	sigaction(SIGINT, &sig_old_int, NULL);
	sigaction(SIGQUIT, &sig_old_quit, NULL);

#ifdef DEBUG
	fprintf(stderr, "RETCODE: %d\n", WEXITSTATUS(res));
#endif /* DEBUG */
	return (0 == res) ? BOOL_TRUE : BOOL_FALSE;
}

/*--------------------------------------------------------- */
bool_t clish_dryrun_callback(clish_shell_t * this,
	const clish_command_t * cmd, const char *script)
{
#ifdef DEBUG
	fprintf(stderr, "DRY-RUN: %s\n", script);
#endif /* DEBUG */

	return BOOL_TRUE;
}

/*--------------------------------------------------------- */
