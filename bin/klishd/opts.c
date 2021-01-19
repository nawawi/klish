#define _GNU_SOURCE
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <syslog.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <getopt.h>
#include <time.h>

#include <faux/faux.h>
#include <faux/str.h>
#include <faux/ini.h>
#include <faux/log.h>
#include <faux/conv.h>

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
	opts->pidfile = faux_str_dup(DEFAULT_PIDFILE);
	opts->cfgfile = faux_str_dup(DEFAULT_CFGFILE);
	opts->unix_socket_path = faux_str_dup(KLISH_DEFAULT_UNIX_SOCKET_PATH);
	opts->cfgfile_userdefined = BOOL_FALSE;
	opts->foreground = BOOL_FALSE; // Daemonize by default
	opts->verbose = BOOL_FALSE;
	opts->log_facility = LOG_DAEMON;

	return opts;
}


/** @brief Free options structure
 */
void opts_free(struct options *opts)
{
	faux_str_free(opts->pidfile);
	faux_str_free(opts->cfgfile);
	faux_str_free(opts->unix_socket_path);
	faux_free(opts);
}


/** @brief Parse command line options
 */
int opts_parse(int argc, char *argv[], struct options *opts)
{
	static const char *shortopts = "hp:c:fl:v";
	static const struct option longopts[] = {
		{"help",		0, NULL, 'h'},
		{"pid",			1, NULL, 'p'},
		{"conf",		1, NULL, 'c'},
		{"foreground",		0, NULL, 'f'},
		{"verbose",		0, NULL, 'v'},
		{"facility",		1, NULL, 'l'},
		{NULL,			0, NULL, 0}
	};

	optind = 1;
	while(1) {
		int opt = 0;

		opt = getopt_long(argc, argv, shortopts, longopts, NULL);
		if (-1 == opt)
			break;
		switch (opt) {
		case 'p':
			faux_str_free(opts->pidfile);
			opts->pidfile = faux_str_dup(optarg);
			break;
		case 'c':
			faux_str_free(opts->cfgfile);
			opts->cfgfile = faux_str_dup(optarg);
			opts->cfgfile_userdefined = BOOL_TRUE;
			break;
		case 'f':
			opts->foreground = BOOL_TRUE;
			break;
		case 'v':
			opts->verbose = BOOL_TRUE;
			break;
		case 'l':
			if (faux_log_facility_id(optarg, &(opts->log_facility))) {
				fprintf(stderr, "Error: Illegal syslog facility %s.\n", optarg);
				_exit(-1);
			}
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
		printf("Klish daemon\n");
		printf("Options :\n");
		printf("\t-h, --help Print this help.\n");
		printf("\t-f, --foreground Don't daemonize.\n");
		printf("\t-v, --verbose Be verbose.\n");
		printf("\t-p <path>, --pid=<path> File to save daemon's PID to ("
			DEFAULT_PIDFILE ").\n");
		printf("\t-c <path>, --conf=<path> Config file ("
			DEFAULT_CFGFILE ").\n");
		printf("\t-l, --facility Syslog facility (DAEMON).\n");
	}
}


/** @brief Parse config file
 */
int config_parse(const char *cfgfile, struct options *opts)
{
	faux_ini_t *ini = NULL;
	const char *tmp = NULL;

	ini = faux_ini_new();
	assert(ini);
	if (!ini)
		return -1;
	if (!faux_ini_parse_file(ini, cfgfile)) {
		syslog(LOG_ERR, "Can't parse config file: %s\n", cfgfile);
		faux_ini_free(ini);
		return -1;
	}

	if ((tmp = faux_ini_find(ini, "UnixSocketPath"))) {
		faux_str_free(opts->unix_socket_path);
		opts->unix_socket_path = faux_str_dup(tmp);
	}

	faux_ini_free(ini);
	return 0;
}


/** @brief Show options. For debug purposes.
 */
int opts_show(struct options *opts)
{
	assert(opts);
	if (!opts)
		return -1;

	syslog(LOG_DEBUG, "opts: Foreground = %s\n", opts->foreground ? "true" : "false");
	syslog(LOG_DEBUG, "opts: Verbose = %s\n", opts->verbose ? "true" : "false");
	syslog(LOG_DEBUG, "opts: LogFacility = %s\n", faux_log_facility_str(opts->log_facility));
	syslog(LOG_DEBUG, "opts: PIDPath = %s\n", opts->pidfile);
	syslog(LOG_DEBUG, "opts: ConfigPath = %s\n", opts->cfgfile);
	syslog(LOG_DEBUG, "opts: UnixSocketPath = %s\n", opts->unix_socket_path);

	return 0;
}
