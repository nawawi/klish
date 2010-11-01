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

#include "private.h"

/*--------------------------------------------------------- */
bool_t clish_script_callback(const clish_shell_t * this,
	const clish_command_t * cmd, const char *script)
{
	const char * shebang = NULL;
	int pipefd[2];
	pid_t cpid;
	char buf;
	int res;

	/* Signal vars */
	struct sigaction sig_old_int;
	struct sigaction sig_old_quit;
	struct sigaction sig_new;
	sigset_t sig_set;

	assert(this);
	assert(cmd);
	if (!script) /* Nothing to do */
		return BOOL_TRUE;

	shebang = clish_command__get_shebang(cmd);
	if (!shebang)
		shebang = clish_shell__get_default_shebang(this);
	assert(shebang);
#ifdef DEBUG
	fprintf(stderr, "SHEBANG: #!%s\n", shebang);
	fprintf(stderr, "SCRIPT: %s\n", script);
#endif /* DEBUG */

	if (pipe(pipefd) < 0)
		return BOOL_FALSE;

	/* Ignore SIGINT and SIGQUIT */
	sigemptyset(&sig_set);
	sig_new.sa_flags = 0;
	sig_new.sa_mask = sig_set;
	sig_new.sa_handler = SIG_IGN;
	sigaction(SIGINT, &sig_new, &sig_old_int);
	sigaction(SIGQUIT, &sig_new, &sig_old_quit);

	/* Create process to execute script */
	cpid = fork();
	if (cpid == -1) {
		close(pipefd[0]);
		close(pipefd[1]);
		return BOOL_FALSE;
	}

	/* Child */
	if (cpid == 0) {
		FILE *wpipe;
		int retval;
		int so; /* saved stdout */
		close(pipefd[0]); /* Close unused read end */
		so = dup(STDOUT_FILENO);
		dup2(pipefd[1], STDOUT_FILENO);
		close(pipefd[1]); /* Close write end */
		wpipe = popen(shebang, "w");
		dup2(so, STDOUT_FILENO);
		close(so);
		if (!wpipe)
			_exit(-1);
		fwrite(script, strlen(script) + 1, 1, wpipe);
		retval = pclose(wpipe);
		_exit(WEXITSTATUS(retval));
	}

	/* Parent */

	/* Read the result of script execution */
	close(pipefd[1]); /* Close unused write end */
	while (read(pipefd[0], &buf, 1) > 0)
		write(fileno(clish_shell__get_ostream(this)), &buf, 1);
	close(pipefd[0]);
	waitpid(cpid, &res, 0);

	/* Restore SIGINT and SIGQUIT */
	sigaction(SIGINT, &sig_old_int, NULL);
	sigaction(SIGQUIT, &sig_old_quit, NULL);

#ifdef DEBUG
	fprintf(stderr, "RETCODE: %d\n", WEXITSTATUS(res));
#endif /* DEBUG */
	return (0 == res) ? BOOL_TRUE : BOOL_FALSE;
}

/*--------------------------------------------------------- */
bool_t clish_dryrun_callback(const clish_shell_t * this,
	const clish_command_t * cmd, const char *script)
{
#ifdef DEBUG
	fprintf(stderr, "DRY-RUN: %s\n", script);
#endif /* DEBUG */

	return BOOL_TRUE;
}

/*--------------------------------------------------------- */
