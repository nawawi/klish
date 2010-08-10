/*
 * clish_startup.c
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif				/* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif				/* DMALLOC */

#include "private.h"

/*--------------------------------------------------------- */
static void usage(const char *filename)
{
	printf("%s [-help] [scriptname]\n", filename);
	printf("  -help      : display this usage\n");
	printf("  scriptname : run the commands in the specified file\n");
	printf("\n");
	printf("VERSION %s\n\n", PACKAGE_VERSION);
	printf("ENVIRONMENT\n");
	printf
	    ("  CLISH_PATH : Set to a semicolon separated list of directories\n");
	printf
	    ("               which should be searched for XML definition files.\n");
	printf("               Current Value: '%s'\n", getenv("CLISH_PATH"));
	printf
	    ("               If undefined then '/etc/clish;~/.clish' will be used.\n");
}

/*--------------------------------------------------------- */
void clish_startup(int argc, const char **argv)
{
#ifdef DMALLOC
	/*
	 * Get environ variable DMALLOC_OPTIONS and pass the settings string
	 * on to dmalloc_debug_setup to setup the dmalloc debugging flags.
	 */
	dmalloc_debug_setup(getenv("DMALLOC_OPTIONS"));
#endif

	if (argc > 1) {
		const char *help_switch = "-help";
		if (strstr(help_switch, argv[1]) == help_switch) {
			usage(argv[0]);
			exit(1);
		}
	}
}

/*--------------------------------------------------------- */
