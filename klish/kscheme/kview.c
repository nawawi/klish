#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <faux/list.h>
#include <faux/error.h>
#include <klish/khelper.h>
#include <klish/kcommand.h>
#include <klish/kview.h>

struct kview_s {
	char *name;
	faux_list_t *commands;
};


// Simple attributes

// Name
KGET_STR(view, name);

// COMMAND list
KCMP_NESTED(view, command, name);
KCMP_NESTED_BY_KEY(view, command, name);
KADD_NESTED(view, command);
KFIND_NESTED(view, command);


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

	view->commands = faux_list_new(FAUX_LIST_SORTED, FAUX_LIST_UNIQUE,
		kview_command_compare, kview_command_kcompare,
		(void (*)(void *))kcommand_free);
	assert(view->commands);

	return view;
}


void kview_free(kview_t *view)
{
	if (!view)
		return;

	faux_str_free(view->name);
	faux_list_free(view->commands);

	faux_free(view);
}
