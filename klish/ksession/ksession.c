/** @file ksession.c
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <faux/list.h>
#include <klish/khelper.h>
#include <klish/kcommand.h>
#include <klish/kparam.h>
#include <klish/ksession.h>


struct ksession_s {
	kscheme_t *scheme;
	kpath_t *path;
};


// Scheme
KGET(session, kscheme_t *, scheme);

// Path
KGET(session, kpath_t *, path);


ksession_t *ksession_new()
{
	ksession_t *session = NULL;

	session = faux_zmalloc(sizeof(*session));
	assert(session);
	if (!session)
		return NULL;

	// Parsed arguments list
	session->pargs = faux_list_new(FAUX_LIST_UNSORTED, FAUX_LIST_NONUNIQUE,
		NULL, NULL, (void (*)(void *))kparg_free);
	assert(session->pargs);

	return session;
}


void ksession_free(ksession_t *session)
{
	if (!session)
		return;

	faux_list_free(session->pargs);

	free(session);
}




bool_t ksession_parse_command(const kscheme_t scheme, 