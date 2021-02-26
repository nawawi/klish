#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <faux/list.h>
#include <klish/khelper.h>
#include <klish/kptype.h>
#include <klish/kview.h>
#include <klish/kscheme.h>


struct kscheme_s {
	faux_list_t *ptypes;
	faux_list_t *views;
};

// Simple methods

// PTYPE list
KCMP_NESTED(scheme, ptype, name);
KCMP_NESTED_BY_KEY(scheme, ptype, name);
KADD_NESTED(scheme, ptype);
KFIND_NESTED(scheme, ptype);

// VIEW list
KCMP_NESTED(scheme, view, name);
KCMP_NESTED_BY_KEY(scheme, view, name);
KADD_NESTED(scheme, view);
KFIND_NESTED(scheme, view);


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

	// PTYPE list
	scheme->ptypes = faux_list_new(FAUX_LIST_SORTED, FAUX_LIST_UNIQUE,
		kscheme_ptype_compare, kscheme_ptype_kcompare,
		(void (*)(void *))kptype_free);
	assert(scheme->ptypes);

	// VIEW list
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

	faux_list_free(scheme->ptypes);
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
