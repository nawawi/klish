//-------------------------------------
// clish.cpp
//
// A simple client for libclish
//-------------------------------------

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>

#include "clish/shell.h"
#include "clish/internal.h"

#ifndef VERSION
#define VERSION 1.2.2
#endif
#define QUOTE(t) #t
#define version(v) printf("%s\n", QUOTE(v))

static clish_shell_hooks_t my_hooks = {
    NULL, /* don't worry about init callback */
    clish_access_callback,
    NULL, /* don't worry about cmd_line callback */
    clish_script_callback,
    NULL, /* don't worry about fini callback */
    clish_config_callback,
    NULL  /* don't register any builtin functions */
};

static void help(int status, const char *argv0);

/*--------------------------------------------------------- */
int main(int argc, char **argv)
{
	bool_t running;
	int result = -1;
	clish_shell_t * shell;

	/* Command line options */
	const char *socket_path = KONFD_SOCKET_PATH;
	bool_t lockless = BOOL_FALSE;
	bool_t stop_on_error = BOOL_FALSE;

	static const char *shortopts = "hvs:le";
/*	static const struct option longopts[] = {
		{"help",	0, NULL, 'h'},
		{"version",	0, NULL, 'v'},
		{"socket",	1, NULL, 's'},
		{"lockless",	0, NULL, 'l'},
		{"stop-on-error", 0, NULL, 'e'},
		{NULL,		0, NULL, 0}
	};
*/
	/* Parse command line options */
	optind = 0;
	while(1) {
		int opt;
/*		opt = getopt_long(argc, argv, shortopts, longopts, NULL); */
		opt = getopt(argc, argv, shortopts);
		if (-1 == opt)
			break;
		switch (opt) {
		case 's':
			socket_path = optarg;
			break;
		case 'l':
			lockless = BOOL_TRUE;
			break;
		case 'e':
			stop_on_error = BOOL_TRUE;
			break;
		case 'h':
			help(0, argv[0]);
			exit(0);
			break;
		case 'v':
			version(VERSION);
			exit(0);
			break;
		default:
			help(-1, argv[0]);
			exit(-1);
			break;
		}
	}

	shell = clish_shell_new(&my_hooks, NULL, NULL, stdout, stop_on_error);
	if (!shell) {
		fprintf(stderr, "Cannot run clish.\n");
		return -1;
	}
	/* Load the XML files */
	clish_shell_load_files(shell);
	/* Set communication to the konfd */
	clish_shell__set_socket(shell, socket_path);
	/* Set lockless mode */
	if (lockless)
		clish_shell__set_lockfile(shell, NULL);
	/* Execute startup */
	running = clish_shell_startup(shell);
	if (!running) {
		fprintf(stderr, "Cannot startup clish.\n");
		clish_shell_delete(shell);
		return -1;
	}

	if(optind < argc) {
		int i;
		/* Run the commands from the files */
		for (i = argc - 1; i >= optind; i--)
			clish_shell_push_file(shell, argv[i], stop_on_error);
	} else {
		/* The interactive shell */
		clish_shell_push_fd(shell, fdopen(dup(fileno(stdin)), "r"), stop_on_error);
	}
	result = clish_shell_loop(shell);

	/* Cleanup */
	clish_shell_delete(shell);

	return result ? 0 : -1;
}

/*--------------------------------------------------------- */
/* Print help message */
static void help(int status, const char *argv0)
{
	const char *name = NULL;

	if (!argv0)
		return;

	/* Find the basename */
	name = strrchr(argv0, '/');
	if (name)
		name++;
	else
		name = argv0;

	if (status != 0) {
		fprintf(stderr, "Try `%s -h' for more information.\n",
			name);
	} else {
		printf("Usage: %s [options]\n", name);
		printf("CLI utility. "
			"The part of the klish project.\n");
		printf("Options:\n");
		printf("\t-v, --version\tPrint version.\n");
		printf("\t-h, --help\tPrint this help.\n");
		printf("\t-s <path>, --socket=<path>\tSpecify listen socket "
			"of the konfd daemon.\n");
		printf("\t-l, --lockless\tDon't use locking mechanism.\n");
		printf("\t-e, --stop-on-error\tStop programm execution on error.\n");
	}
}

