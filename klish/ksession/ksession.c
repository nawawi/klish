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


ksession_t *ksession_new(const kscheme_t *scheme, const char *start_entry)
{
	ksession_t *session = NULL;
	kentry_t *entry = NULL;
	const char *entry_to_search = NULL;
	klevel_t *level = NULL;

	assert(scheme);
	if (!scheme)
		return NULL;

	// Before real session allocation we will try to find starting entry.
	// Starting entry can be get from function argument, from STARTUP tag or
	// default name 'main' can be used. Don't create session if we can't get
	// starting entry at all. Priorities are (from higher) argument, STARTUP,
	// default name.
	if (start_entry)
		entry_to_search = start_entry;
	// STARTUP is not implemented yet
	else
		entry_to_search = KSESSION_STARTING_ENTRY;
	entry = kscheme_find_entry_by_path(scheme, entry_to_search);
	if (!entry)
		return NULL; // Can't find starting entry

	session = faux_zmalloc(sizeof(*session));
	assert(session);
	if (!session)
		return NULL;

	// Initialization
	session->scheme = scheme;
	// Create kpath_t stack
	session->path = kpath_new();
	assert(session->path);
	level = klevel_new(entry);
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
