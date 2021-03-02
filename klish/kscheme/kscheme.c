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


bool_t kscheme_nested_from_ischeme(kscheme_t *kscheme, ischeme_t *ischeme,
	faux_error_t *error_stack)
{
	bool_t retval = BOOL_TRUE;

	if (!kscheme || !ischeme) {
		faux_error_add(error_stack,
			kscheme_strerror(KSCHEME_ERROR_INTERNAL));
		return BOOL_FALSE;
	}

	// PTYPE list
	if (ischeme->ptypes) {
		iptype_t **p_iptype = NULL;
		for (p_iptype = *ischeme->ptypes; *p_iptype; p_iptype++) {
			kptype_t *kptype = NULL;
			iptype_t *iptype = *p_iptype;

			kptype = kptype_from_iptype(iptype, error_stack);
			if (!kptype) {
				retval = BOOL_FALSE; // Don't stop
				continue;
			}
			if (!kscheme_add_ptype(kscheme, kptype)) {
				// Search for PTYPE duplicates
				if (kscheme_find_ptype(kscheme,
					kptype_name(kptype))) {
					faux_error_sprintf(error_stack,
						"SCHEME: "
						"Can't add duplicate PTYPE "
						"\"%s\"",
						kptype_name(kptype));
				} else {
					faux_error_sprintf(error_stack,
						"SCHEME: "
						"Can't add PTYPE \"%s\"",
						kptype_name(kptype));
				}
				kptype_free(kptype);
				retval = BOOL_FALSE;
			}
		}
	}

	// VIEW list
	// VIEW entries can be duplicate. Duplicated entries will add nested
	// elements to existent VIEW. Also it can overwrite VIEW attributes.
	// So there is no special rule which attribute value will be "on top".
	// It's a random. Technically later VIEW entries will rewrite previous
	// values.
	if (ischeme->views) {
		iview_t **p_iview = NULL;
		for (p_iview = *ischeme->views; *p_iview; p_iview++) {
			kview_t *kview = NULL;
			iview_t *iview = *p_iview;
			const char *view_name = iview->name;

			if (view_name)
				kview = kscheme_find_view(kscheme, view_name);

			// VIEW already exists
			if (kview) {
				kview_error_e kview_error = KVIEW_ERROR_OK;
				if (!kview_parse(kview, iview, &kview_error)) {
					faux_error_sprintf(error_stack,
						"VIEW \"%s\": %s",
						iview->name ? iview->name : "(null)",
						kview_strerror(kview_error));
					retval = BOOL_FALSE;
					continue;
				}
				if (!kview_nested_from_iview(kview, iview,
					error_stack)) {
					retval = BOOL_FALSE;
					continue;
				}
				continue;
			}

			// New VIEW
			kview = kview_from_iview(iview, error_stack);
			if (!kview) {
				retval = BOOL_FALSE;
				continue;
			}
			if (!kscheme_add_view(kscheme, kview)) {
				faux_error_sprintf(error_stack,
					"SCHEME: "
					"Can't add VIEW \"%s\"",
					kview_name(kview));
				kview_free(kview);
				retval = BOOL_FALSE;
				continue;
			}
		}
	}

	if (!retval)
		faux_error_sprintf(error_stack, "SCHEME: Illegal nested elements");

	return retval;
}


kscheme_t *kscheme_from_ischeme(ischeme_t *ischeme, faux_error_t *error_stack)
{
	kscheme_t *kscheme = NULL;
	kscheme_error_e kscheme_error = KSCHEME_ERROR_OK;

	kscheme = kscheme_new(&kscheme_error);
	if (!kscheme) {
		faux_error_sprintf(error_stack, "SCHEME: %s",
			kscheme_strerror(kscheme_error));
		return NULL;
	}

	// Parse nested elements
	if (!kscheme_nested_from_ischeme(kscheme, ischeme, error_stack)) {
		kscheme_free(kscheme);
		return NULL;
	}

	return kscheme;
}
