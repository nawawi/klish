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
	opts->socket = faux_str_dup(KLISH_DEFAULT_UNIX_SOCKET_PATH);

	return opts;
}


/** @brief Free options structure
 */
void opts_free(struct options *opts)
{
	if (!opts)
		return;
	faux_str_free(opts->socket);
	faux_free(opts);
}


/** @brief Parse command line options
 */
int opts_parse(int argc, char *argv[], struct options *opts)
{
	static const char *shortopts = "hvS:";
	static const struct option longopts[] = {
		{"socket",		1, NULL, 'S'},
		{"help",		0, NULL, 'h'},
		{"verbose",		0, NULL, 'v'},
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
			faux_str_free(opts->socket);
			opts->socket = faux_str_dup(optarg);
			break;
		case 'v':
			opts->verbose = BOOL_TRUE;
			break;
		case 'h':
			help(0, argv[0]);
			_exit(0);
			break;
		default:
			help(-1, argv[0]);
			_exit(-1);
			break;
		}
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
		printf("Usage   : %s [options]\n", name);
		printf("Klish client\n");
		printf("Options :\n");
		printf("\t-S, --socket UNIX socket path.\n");
		printf("\t-h, --help Print this help.\n");
		printf("\t-v, --verbose Be verbose.\n");
	}
}
