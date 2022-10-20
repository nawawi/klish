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


static char *shell_mkfifo(void)
{
	int res = 0;
	char *name = NULL;

	name = faux_str_sprintf("/tmp/klish.fifo.%u.XXXXXX", getpid());
	mktemp(name);
	if (mkfifo(name, 0600) < 0) {
		faux_str_free(name);
		return NULL;
	}

	return name;
}


// Execute shell script
int shell_shell(kcontext_t *context)
{
	const char *script = NULL;
	pid_t cpid = -1;
	int res = -1;
	char *fifo_name = NULL;
	FILE *wpipe = NULL;
	char *command = NULL;

	script = kcontext_script(context);
	if (faux_str_is_empty(script))
		return 0;

	// Create FIFO
	if (!(fifo_name = shell_mkfifo())) {
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
		if (!wpipe)
			_exit(-1);
		fwrite(script, strlen(script), 1, wpipe);
		fflush(wpipe);
		fclose(wpipe);
		_exit(0);
	}

	// Parent
	// Prepare command
	command = faux_str_sprintf("/bin/sh %s", fifo_name);

	res = system(command);

	// Wait for the writing process
	kill(cpid, SIGTERM);
	while (waitpid(cpid, NULL, 0) != cpid);

	// Clean up
	faux_str_free(command);
	unlink(fifo_name);
	faux_str_free(fifo_name);

	return WEXITSTATUS(res);
}
