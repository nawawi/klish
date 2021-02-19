#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <faux/list.h>
#include <klish/kcommand.h>
#include <klish/kview.h>


struct kview_s {
	bool_t is_static;
	iview_t info;
	faux_list_t *commands;
};


static int kview_command_compare(const void *first, const void *second)
{
	const kcommand_t *f = (const kcommand_t *)first;
	const kcommand_t *s = (const kcommand_t *)second;

	return strcmp(kcommand_name(f), kcommand_name(s));
}


static int kview_command_kcompare(const void *key, const void *list_item)
{
	const char *f = (const char *)key;
	const kcommand_t *s = (const kcommand_t *)list_item;

	return strcmp(f, kcommand_name(s));
}


static kview_t *kview_new_internal(iview_t info, bool_t is_static)
{
	kview_t *view = NULL;

	view = faux_zmalloc(sizeof(*view));
	assert(view);
	if (!view)
		return NULL;

	// Initialize
	view->is_static = is_static;
	view->info = info;

	view->commands = faux_list_new(FAUX_LIST_SORTED, FAUX_LIST_UNIQUE,
		kview_command_compare, kview_command_kcompare,
		(void (*)(void *))kcommand_free);
	assert(view->commands);

	return view;
}


kview_t *kview_new(iview_t info)
{
	return kview_new_internal(info, BOOL_FALSE);
}


kview_t *kview_new_static(iview_t info)
{
	return kview_new_internal(info, BOOL_TRUE);
}


void kview_free(kview_t *view)
{
	if (!view)
		return;

	if (!view->is_static) {
		faux_str_free(view->info.name);
	}
	faux_list_free(view->commands);

	faux_free(view);
}


const char *kview_name(const kview_t *view)
{
	assert(view);
	if (!view)
		return NULL;

	return view->info.name;
}


bool_t kview_add_command(kview_t *view, kcommand_t *command)
{
	assert(view);
	if (!view)
		return BOOL_FALSE;
	assert(command);
	if (!command)
		return BOOL_FALSE;

	if (!faux_list_add(view->commands, command))
		return BOOL_FALSE;

	return BOOL_TRUE;
}
