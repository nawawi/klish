#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <faux/list.h>
#include <klish/kview.h>
#include <klish/kscheme.h>


struct kscheme_s {
	faux_list_t *views;
};


static int kscheme_view_compare(const void *first, const void *second)
{
	const kview_t *f = (const kview_t *)first;
	const kview_t *s = (const kview_t *)second;

	return strcmp(kview_name(f), kview_name(s));
}


static int kscheme_view_kcompare(const void *key, const void *list_item)
{
	const char *f = (const char *)key;
	const kview_t *s = (const kview_t *)list_item;

	return strcmp(f, kview_name(s));
}


kscheme_t *kscheme_new(kscheme_error_e *error)
{
	kscheme_t *scheme = NULL;

	scheme = faux_zmalloc(sizeof(*scheme));
	assert(scheme);
	if (!scheme) {
		if (error)
			*error = KSCHEME_ERROR_ALLOC;
		return NULL;
	}

	// Initialize
	scheme->views = faux_list_new(FAUX_LIST_SORTED, FAUX_LIST_UNIQUE,
		kscheme_view_compare, kscheme_view_kcompare,
		(void (*)(void *))kview_free);
	assert(scheme->views);

	return scheme;
}


void kscheme_free(kscheme_t *scheme)
{
	if (!scheme)
		return;

	faux_list_free(scheme->views);
	faux_free(scheme);
}


const char *kscheme_strerror(kscheme_error_e error)
{
	const char *str = NULL;

	switch (error) {
	case KSCHEME_ERROR_OK:
		str = "Ok";
		break;
	case KSCHEME_ERROR_INTERNAL:
		str = "Internal error";
		break;
	case KVIEW_ERROR_ALLOC:
		str = "Memory allocation error";
		break;
	default:
		str = "Unknown error";
		break;
	}

	return str;
}


bool_t kscheme_add_view(kscheme_t *scheme, kview_t *view)
{
	assert(scheme);
	if (!scheme)
		return BOOL_FALSE;
	assert(view);
	if (!view)
		return BOOL_FALSE;

	if (!faux_list_add(scheme->views, view))
		return BOOL_FALSE;

	return BOOL_TRUE;
}


kview_t *kscheme_find_view(const kscheme_t *scheme, const char *name)
{
	assert(scheme);
	if (!scheme)
		return NULL;
	assert(name);
	if (!name)
		return NULL;

	return (kview_t *)faux_list_kfind(scheme->views, name);
}
