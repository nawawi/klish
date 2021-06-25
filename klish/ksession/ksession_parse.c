/** @file ksession_parse.c
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <faux/argv.h>
#include <klish/khelper.h>
#include <klish/kview.h>
#include <klish/kscheme.h>
#include <klish/kpath.h>
#include <klish/kpargv.h>
#include <klish/ksession.h>


static bool_t ksession_parse_arg(kentry_t *current_entry,
	faux_argv_node_t **argv_iter, kpargv_t *pargv)
{
	assert(current_entry);
	if (!current_entry)
		return BOOL_FALSE;
	assert(argv_iter);
	if (!argv_iter)
		return BOOL_FALSE;
	assert(*argv_iter);
	if (!*argv_iter)
		return BOOL_FALSE;
	assert(pargv);
	if (!pargv)
		return BOOL_FALSE;

	// Is entry candidate to resolve current arg?
	// Which entries can't be a candidate:
	// * container
	if (!kentry_container(current_entry)) {
		const char *current_arg = faux_argv_current(*argv_iter);

		current_arg = current_arg;
	}

	return BOOL_FALSE;
}


kpargv_t *ksession_parse_line(ksession_t *session, const char *line)
{
	faux_argv_t *argv = NULL;
	faux_argv_node_t *argv_iter = NULL;
	kentry_t *current_entry = NULL;
	kpargv_t *pargv = NULL;

	assert(session);
	if (!session)
		return NULL;
	assert(line);
	if (!line)
		return NULL;

	// Split line to arguments
	argv = faux_argv_new();
	assert(argv);
	if (!argv)
		return NULL;
	if (faux_argv_parse(argv, line) <= 0) {
		faux_argv_free(argv);
		return NULL;
	}
	argv_iter = faux_argv_iter(argv);

	current_entry = klevel_entry(kpath_current(ksession_path(session)));
	pargv = kpargv_new();
	assert(pargv);

	ksession_parse_arg(current_entry, &argv_iter, pargv);

	if (kpargv_pargs_is_empty(pargv)) {
		kpargv_free(pargv);
		return NULL;
	}

	return pargv;
}
