#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <faux/list.h>
#include <faux/error.h>
#include <klish/khelper.h>
#include <klish/kcommand.h>
#include <klish/knspace.h>
#include <klish/kview.h>

struct kview_s {
	char *name;
	faux_list_t *commands;
	faux_list_t *nspaces;
};


// Simple attributes

// Name
KGET_STR(view, name);

// COMMAND list
KGET(view, faux_list_t *, commands);
KCMP_NESTED(view, command, name);
KCMP_NESTED_BY_KEY(view, command, name);
KADD_NESTED(view, kcommand_t *, commands);
KFIND_NESTED(view, command);
KNESTED_LEN(view, commands);
KNESTED_ITER(view, commands);
KNESTED_EACH(view, kcommand_t *, commands);

// NSPACE list
KGET(view, faux_list_t *, nspaces);
KADD_NESTED(view, knspace_t *, nspaces);
KNESTED_LEN(view, nspaces);
KNESTED_ITER(view, nspaces);
KNESTED_EACH(view, knspace_t *, nspaces);


kview_t *kview_new(const char *name)
{
	kview_t *view = NULL;

	if (faux_str_is_empty(name))
		return NULL;

	view = faux_zmalloc(sizeof(*view));
	assert(view);
	if (!view)
		return NULL;

	// Initialize
	view->name = faux_str_dup(name);

	// COMMANDs
	view->commands = faux_list_new(FAUX_LIST_SORTED, FAUX_LIST_UNIQUE,
		kview_command_compare, kview_command_kcompare,
		(void (*)(void *))kcommand_free);
	assert(view->commands);

	// NSPACEs
	// The order of NSPACEs is the same as order of NSPACE tags.
	// The NSPACE need not to be unique because each VIEW can be
	// NSPACEd with prefix or without prefix.
	view->nspaces = faux_list_new(FAUX_LIST_UNSORTED, FAUX_LIST_NONUNIQUE,
		NULL, NULL, (void (*)(void *))knspace_free);
	assert(view->nspaces);

	return view;
}


void kview_free(kview_t *view)
{
	if (!view)
		return;

	faux_str_free(view->name);
	faux_list_free(view->commands);
	faux_list_free(view->nspaces);

	faux_free(view);
}
