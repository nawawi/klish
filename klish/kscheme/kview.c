#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <faux/list.h>
#include <klish/khelper.h>
#include <klish/kcommand.h>
#include <klish/kview.h>

struct kview_s {
	char *name;
	faux_list_t *commands;
};


// Simple attributes
KGET_STR(kview, name);
KSET_STR_ONCE(kview, name);


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


static kview_t *kview_new_empty(void)
{
	kview_t *view = NULL;

	view = faux_zmalloc(sizeof(*view));
	assert(view);
	if (!view)
		return NULL;

	// Initialize
	view->name = NULL;

	view->commands = faux_list_new(FAUX_LIST_SORTED, FAUX_LIST_UNIQUE,
		kview_command_compare, kview_command_kcompare,
		(void (*)(void *))kcommand_free);
	assert(view->commands);

	return view;
}


kview_t *kview_new(const iview_t *info, kview_error_e *error)
{
	kview_t *view = NULL;

	view = kview_new_empty();
	assert(view);
	if (!view) {
		if (error)
			*error = KVIEW_ERROR_ALLOC;
		return NULL;
	}

	if (!info)
		return view;

	if (!kview_parse(view, info, error)) {
		kview_free(view);
		return NULL;
	}

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


const char *kview_strerror(kview_error_e error)
{
	const char *str = NULL;

	switch (error) {
	case KVIEW_ERROR_OK:
		str = "Ok";
		break;
	case KVIEW_ERROR_ALLOC:
		str = "Memory allocation error";
		break;
	case KVIEW_ERROR_ATTR_NAME:
		str = "Illegal attribute \"name\"";
		break;
	default:
		str = "Unknown error";
		break;
	}

	return str;
}


bool_t kview_parse(kview_t *view, const iview_t *info, kview_error_e *error)
{
	view = view;
	info = info;
	error = error;

	// Name
	if (!faux_str_is_empty(info->name))
		if (!kview_set_name(view, info->name)) {
			if (error)
				*error = KVIEW_ERROR_ATTR_NAME;
			return BOOL_FALSE;
	}

	return BOOL_TRUE;
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
