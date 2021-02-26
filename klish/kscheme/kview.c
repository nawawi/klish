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

// Name
KGET_STR(view, name);
KSET_STR_ONCE(view, name);

// COMMAND list
KCMP_NESTED(view, command, name);
KCMP_NESTED_BY_KEY(view, command, name);
KADD_NESTED(view, command);
KFIND_NESTED(view, command);


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
	case KVIEW_ERROR_INTERNAL:
		str = "Internal error";
		break;
	case KVIEW_ERROR_ALLOC:
		str = "Memory allocation error";
		break;
	case KVIEW_ERROR_ATTR_NAME:
		str = "Illegal 'name' attribute";
		break;
	default:
		str = "Unknown error";
		break;
	}

	return str;
}


bool_t kview_parse(kview_t *view, const iview_t *info, kview_error_e *error)
{
	bool_t retval = BOOL_TRUE;

	// Name [mandatory]
	if (faux_str_is_empty(info->name)) {
		if (error)
			*error = KVIEW_ERROR_ATTR_NAME;
		retval = BOOL_FALSE;
	} else {
		if (!kview_set_name(view, info->name)) {
			if (error)
				*error = KVIEW_ERROR_ATTR_NAME;
			retval = BOOL_FALSE;
		}
	}

	return retval;
}
