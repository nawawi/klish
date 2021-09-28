#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <getopt.h>

#include <faux/faux.h>
#include <faux/str.h>
#include <faux/list.h>

#include <klish/ktp_session.h>

#include "private.h"


/** @brief Initialize option structure by defaults
 */
struct options *opts_init(void)
{
	struct options *opts = NULL;

	opts = faux_zmalloc(sizeof(*opts));
	assert(opts);

	// Initialize
	opts->verbose = BOOL_FALSE;
	opts->stop_on_error = BOOL_FALSE;
	opts->dry_run = BOOL_FALSE;
	opts->quiet = BOOL_FALSE;
	opts->unix_socket_path = faux_str_dup(KLISH_DEFAULT_UNIX_SOCKET_PATH);

	// Don't free command list because elements are the pointers to
	// command line options and don't need to be freed().
	opts->commands = faux_list_new(FAUX_LIST_UNSORTED, FAUX_LIST_NONUNIQUE,
		NULL, NULL, NULL);
	opts->files = faux_list_new(FAUX_LIST_UNSORTED, FAUX_LIST_NONUNIQUE,
		NULL, NULL, NULL);

	return opts;
}


/** @brief Free options structure
 */
void opts_free(struct options *opts)
{
	if (!opts)
		return;
	faux_str_free(opts->unix_socket_path);
	faux_list_free(opts->commands);
	faux_list_free(opts->files);

	faux_free(opts);
}


/** @brief Parse command line options
 */
int opts_parse(int argc, char *argv[], struct options *opts)
{
	static const char *shortopts = "hvS:c:edq";
	static const struct option longopts[] = {
		{"socket",		1, NULL, 'S'},
		{"help",		0, NULL, 'h'},
		{"verbose",		0, NULL, 'v'},
		{"command",		1, NULL, 'c'},
		{"stop-on-error",	0, NULL, 'e'},
		{"dry-run",		0, NULL, 'd'},
		{"quiet",		0, NULL, 'q'},
		{NULL,			0, NULL, 0}
	};

	optind = 1;
	while(1) {
		int opt = 0;

		opt = getopt_long(argc, argv, shortopts, longopts, NULL);
		if (-1 == opt)
			break;
		switch (opt) {
		case 'S':
			faux_str_free(opts->unix_socket_path);
			opts->unix_socket_path = faux_str_dup(optarg);
			break;
		case 'v':
			opts->verbose = BOOL_TRUE;
			break;
		case 'e':
			opts->stop_on_error = BOOL_TRUE;
			break;
		case 'd':
			opts->dry_run = BOOL_TRUE;
			break;
		case 'q':
			opts->quiet = BOOL_TRUE;
			break;
		case 'h':
			help(0, argv[0]);
			_exit(0);
			break;
		case 'c':
			faux_list_add(opts->commands, optarg);
			break;
		default:
			help(-1, argv[0]);
			_exit(-1);
			break;
		}
	}

	// Input files
	if(optind < argc) {
		int i;
		for (i = argc - 1; i >= optind; i--)
			faux_list_add(opts->files, argv[i]);
	}

	// Validate options

	// Commands specified by '-c' option can't coexist with input files
	if (!faux_list_is_empty(opts->commands) &&
		!faux_list_is_empty(opts->files)) {
		fprintf(stderr, "Error: Commands specified by '-c' can't coexist "
			"with input files\n");
		_exit(-1);
	}

	return 0;
}

/** @brief Print help message
 */
void help(int status, const char *argv0)
{
	const char *name = NULL;

	if (!argv0)
		return;

	// Find the basename
	name = strrchr(argv0, '/');
	if (name)
		name++;
	else
		name = argv0;

	if (status != 0) {
		fprintf(stderr, "Try `%s -h' for more information.\n",
			name);
	} else {
		printf("Version : %s\n", VERSION);
		printf("Usage   : %s [options] [filename] ... [filename]\n", name);
		printf("Klish client\n");
		printf("Options :\n");
		printf("\t-S <path>, --socket=<path> UNIX socket path.\n");
		printf("\t-h, --help Print this help.\n");
		printf("\t-v, --verbose Be verbose.\n");
		printf("\t-c <line>, --command=<line> Command to execute.\n"
			"\t\tMultiple options are allowed.\n");
		printf("\t-e, --stop-on-error Stop script execution on error.\n");
		printf("\t-q, --quiet Disable echo while executing commands\n\t\tfrom the file stream.\n");
		printf("\t-d, --dry-run Don't actually execute ACTION scripts.\n");
	}
}
