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

typedef enum {
	KPARSE_NONE,
	KPARSE_OK,
	KPARSE_INPROGRESS,
	KPARSE_NOTFOUND,
	KPARSE_INCOMPLETED,
	KPARSE_ILLEGAL,
	KPARSE_ERROR,
} kparse_status_e;


static bool_t ksession_validate_arg(kentry_t *entry, const char *arg)
{
	assert(entry);
	if (!entry)
		return BOOL_FALSE;
	assert(arg);
	if (!arg)
		return BOOL_FALSE;


	return BOOL_FALSE;
}


static kparse_status_e ksession_parse_arg(kentry_t *current_entry,
	faux_argv_node_t **argv_iter, kpargv_t *pargv)
{
	kentry_t *entry = current_entry;
	kentry_mode_e mode = KENTRY_MODE_NONE;
	kparse_status_e retcode = KPARSE_NONE;

	assert(current_entry);
	if (!current_entry)
		return KPARSE_ERROR;
	assert(argv_iter);
	if (!argv_iter)
		return KPARSE_ERROR;
	assert(*argv_iter);
	if (!*argv_iter)
		return KPARSE_ERROR;
	assert(pargv);
	if (!pargv)
		return KPARSE_ERROR;

	// Is entry candidate to resolve current arg?
	// Container can't be a candidate.
	if (!kentry_container(entry)) {
		const char *current_arg = faux_argv_current(*argv_iter);
		if (ksession_validate_arg(entry, current_arg)) {
			kparg_t *parg = kparg_new(entry, current_arg);
			kpargv_add_parg(pargv, parg);
			faux_argv_each(argv_iter); // Next argument
			retcode = KPARSE_INPROGRESS;
		} else {
			// It's not a container and is not validated so
			// no chance to find anything here.
			return KPARSE_NOTFOUND;
		}
	}

	// Walk through the nested entries
	mode = kentry_mode(entry);
	if (KENTRY_MODE_EMPTY == mode)
		return retcode;

	// SWITCH
	// Entries within SWITCH can't has 'min'/'max' else than 1.
	// So these attributes will be ignored. Note SWITCH itself can have
	// 'min'/'max'.
	if (KENTRY_MODE_SWITCH == mode) {
		kentry_entrys_node_t *iter = kentry_entrys_iter(entry);
		kentry_t *nested = NULL;
		while ((nested = kentry_entrys_each(&iter))) {
			if (ksession_parse_arg(nested, argv_iter, pargv)) {
				retcode = KPARSE_INPROGRESS;
				break;
			}
		}

	// SEQUENCE
	} else if (KENTRY_MODE_SEQUENCE == mode) {
		kentry_entrys_node_t *iter = kentry_entrys_iter(entry);
		kentry_t *nested = NULL;
		while ((nested = kentry_entrys_each(&iter))) {
			if (ksession_parse_arg(nested, argv_iter, pargv))
				retcode = KPARSE_INPROGRESS;
		}
	
	}

	return retcode;
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
