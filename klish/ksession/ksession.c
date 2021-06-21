/** @file ksession.c
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


struct ksession_s {
	const kscheme_t *scheme;
	kpath_t *path;
};


// Scheme
KGET(session, const kscheme_t *, scheme);

// Path
KGET(session, kpath_t *, path);


ksession_t *ksession_new(const kscheme_t *scheme, const char *start_view)
{
	ksession_t *session = NULL;
	kview_t *view = NULL;
	const char *view_to_search = NULL;
	klevel_t *level = NULL;

	assert(scheme);
	if (!scheme)
		return NULL;

	// Before real session allocation we will try to find starting view.
	// Starting view can be get from function argument, from STARTUP tag or
	// default name 'main' can be used. Don't create session if we can't get
	// starting view at all. Priorities are (from higher) argument, STARTUP,
	// default name.
	if (start_view)
		view_to_search = start_view;
	// STARTUP is not implemented yet
	else
		view_to_search = KSESSION_DEFAULT_VIEW;
	view = kscheme_find_view(scheme, view_to_search);
	if (view)
		return NULL; // Can't find starting view

	session = faux_zmalloc(sizeof(*session));
	assert(session);
	if (!session)
		return NULL;

	// Initialization
	session->scheme = scheme;
	// Create kpath_t stack
	session->path = kpath_new();
	assert(session->path);
	level = klevel_new(view);
	assert(level);
	kpath_push(session->path, level);

	return session;
}


void ksession_free(ksession_t *session)
{
	if (!session)
		return;

	kpath_free(session->path);

	free(session);
}

/*
kpargv_t *ksession_parse_line(ksession_t session, const faux_argv_t *argv)
{



}
*/
