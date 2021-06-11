#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <faux/list.h>
#include <faux/error.h>
#include <klish/khelper.h>
#include <klish/kview.h>
#include <klish/knspace.h>

struct knspace_s {
	char *view_ref;
	kview_t *view;
	char *prefix;
};


// Simple attributes

// Name
KGET_STR(nspace, view_ref);

// View
KGET(nspace, kview_t *, view);
KSET(nspace, kview_t *, view);

// Prefix
KGET_STR(nspace, prefix);
KSET_STR(nspace, prefix);


knspace_t *knspace_new(const char *view_ref)
{
	knspace_t *nspace = NULL;

	if (faux_str_is_empty(view_ref))
		return NULL;

	nspace = faux_zmalloc(sizeof(*nspace));
	assert(nspace);
	if (!nspace)
		return NULL;

	// Initialize
	nspace->view_ref = faux_str_dup(view_ref);
	nspace->view = NULL;
	nspace->prefix = NULL;

	return nspace;
}


void knspace_free(knspace_t *nspace)
{
	if (!nspace)
		return;

	faux_str_free(nspace->view_ref);
	faux_str_free(nspace->prefix);

	faux_free(nspace);
}
