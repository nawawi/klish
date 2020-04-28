#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>

#if WITH_INTERNAL_GETOPT
#include "libc/getopt.h"
#else
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif
#endif

#if HAVE_LOCALE_H
#include <locale.h>
#endif
#if HAVE_LANGINFO_CODESET
#include <langinfo.h>
#endif

#include "faux/faux.h"
#include "faux/str.h"
#include "faux/list.h"

#ifndef VERSION
#define VERSION 1.0.0
#endif
#define QUOTE(t) #t
#define version(v) printf("%s\n", v)

#define TESTC_LIST_SYM "testc_module"


// Command line options */
struct opts_s {
	int debug;
	faux_list_t *so_list;
};

typedef struct opts_s opts_t;

static opts_t *opts_parse(int argc, char *argv[]);
static void opts_free(opts_t *opts);
static void help(int status, const char *argv0);


int main(int argc, char *argv[]) {

	opts_t *opts = NULL;
	faux_list_node_t *iter = NULL;
	char *so = NULL;
	// Return value will be negative on any error or failed test.
	// It doesn't mean that any error will break the processing.
	// The following var is error counter.
	unsigned int total_errors = 0;
	unsigned int total_modules = 0;
	unsigned int total_tests = 0;

#if HAVE_LOCALE_H
	// Set current locale
	setlocale(LC_ALL, "");
#endif

	// Parse command line options
	opts = opts_parse(argc, argv);
	if (!opts) {
		fprintf(stderr, "Error: Can't parse command line options\n");
		return -1;
	}

	iter = faux_list_head(opts->so_list);
	while ((so = faux_list_each(&iter))) {
		void *so_handle = NULL;
		const char **test_list = NULL;
		const char *test_name = NULL;
		const char *test_desc = NULL;
		unsigned int module_tests = 0;
		unsigned int module_errors = 0;

		total_modules++;
		printf("Processing module \"%s\"...\n", so);
		if (access(so, R_OK) < 0) {
			fprintf(stderr, "Error: Can't read module \"%s\"... Skipped\n", so);
			total_errors++;
			continue;
		}

		so_handle = dlopen(so, RTLD_LAZY | RTLD_LOCAL);
		if (!so_handle) {
			fprintf(stderr, "Error: Can't open module \"%s\"... Skipped\n", so);
			total_errors++;
			continue;
		}

		test_list = (const char **)dlsym(so_handle, TESTC_LIST_SYM);
		while (*test_list) {
			int (*test_sym)(void);
			int retval = 0;
			char *result = NULL;

			test_name = *test_list;
			test_list++;
			if (!*test_list) // Broken test list structure
				break;
			test_desc = *test_list;
			test_list++;
			module_tests++;

			test_sym = (int (*)(void))dlsym(so_handle, test_name);
			if (!test_sym) {
				fprintf(stderr, "Error: Can't find symbol \"%s\"... Skipped\n", test_name);
				module_errors++;
				continue;
			}

			retval = test_sym();
			if (0 == retval) {
				result = "success";
			} else {
				result = "fail";
				module_errors++;
			}
			printf("Test #%03u %s() %s: %s\n", module_tests, test_name, test_desc, result);
		}


		dlclose(so_handle);
		so_handle = NULL;

		printf("Module tests: %u\n", module_tests);
		printf("Module errors: %u\n", module_errors);

		total_tests += module_tests;
		total_errors += module_errors;

	}

	opts_free(opts);

	// Total statistics
	printf("Total modules: %u\n", total_modules);
	printf("Total tests: %u\n", total_tests);
	printf("Total errors: %u\n", total_errors);

	if (total_errors > 0)
		return -1;
	return 0;
}


static void opts_free(opts_t *opts) {

	assert(opts);
	if (!opts)
		return;

	faux_list_free(opts->so_list);
	faux_free(opts);
}


static opts_t *opts_new(void) {

	opts_t *opts = NULL;

	opts = faux_zmalloc(sizeof(*opts));
	assert(opts);
	if (!opts)
		return NULL;

	opts->debug = BOOL_FALSE;

	// Members of list are static strings from argv so don't free() it
	opts->so_list = faux_list_new(BOOL_FALSE, BOOL_TRUE,
		(faux_list_cmp_fn)strcmp, NULL, NULL);
	if (!opts->so_list) {
		opts_free(opts);
		return NULL;
	}

	return opts;
}


static opts_t *opts_parse(int argc, char *argv[]) {

	opts_t *opts = NULL;

	static const char *shortopts = "hvd";
#ifdef HAVE_GETOPT_LONG
	static const struct option longopts[] = {
		{"help",	0, NULL, 'h'},
		{"version",	0, NULL, 'v'},
		{"debug",	0, NULL, 'd'},
		{NULL,		0, NULL, 0}
	};
#endif

	opts = opts_new();
	if (!opts)
		return NULL;

	optind = 1;
	while (1) {
		int opt;
#ifdef HAVE_GETOPT_LONG
		opt = getopt_long(argc, argv, shortopts, longopts, NULL);
#else
		opt = getopt(argc, argv, shortopts);
#endif
		if (-1 == opt)
			break;
		switch (opt) {
		case 'd':
			opts->debug = BOOL_TRUE;
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

	if (optind < argc) {
		int i = 0;
		for (i = optind; i < argc; i++)
			faux_list_add(opts->so_list, argv[i]);
	} else {
		help(-1, argv[0]);
		exit(-1);
	}

	return opts;
}


static void help(int status, const char *argv0) {

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
		printf("Usage: %s [options] <so_object> [so_object] ...\n", name);
		printf("Unit test helper for C code.\n");
		printf("Options:\n");
		printf("\t-v, --version\tPrint version.\n");
		printf("\t-h, --help\tPrint this help.\n");
		printf("\t-d, --debug\tDebug mode. Don't daemonize.\n");
	}
}
