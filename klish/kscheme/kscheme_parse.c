#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <faux/list.h>
#include <klish/khelper.h>
#include <klish/ischeme.h>
#include <klish/kplugin.h>
#include <klish/kptype.h>
#include <klish/kview.h>
#include <klish/kscheme.h>

#define TAG "SCHEME"


bool_t kscheme_nested_from_ischeme(kscheme_t *kscheme, ischeme_t *ischeme,
	faux_error_t *error)
{
	bool_t retval = BOOL_TRUE;

	if (!kscheme || !ischeme) {
		faux_error_add(error, TAG": Internal error");
		return BOOL_FALSE;
	}

	// PLUGIN list
	if (ischeme->plugins) {
		iplugin_t **p_iplugin = NULL;
		for (p_iplugin = *ischeme->plugins; *p_iplugin; p_iplugin++) {
			kplugin_t *kplugin = NULL;
			iplugin_t *iplugin = *p_iplugin;

			kplugin = kplugin_from_iplugin(iplugin, error);
			if (!kplugin) {
				retval = BOOL_FALSE; // Don't stop
				continue;
			}
			if (!kscheme_add_plugin(kscheme, kplugin)) {
				// Search for PLUGIN duplicates
				if (kscheme_find_plugin(kscheme,
					kplugin_name(kplugin))) {
					faux_error_sprintf(error,
						TAG": Can't add duplicate PLUGIN "
						"\"%s\"", kplugin_name(kplugin));
				} else {
					faux_error_sprintf(error,
						TAG": Can't add PLUGIN \"%s\"",
						kplugin_name(kplugin));
				}
				kplugin_free(kplugin);
				retval = BOOL_FALSE;
			}
		}
	}

	// PTYPE list
	if (ischeme->ptypes) {
		iptype_t **p_iptype = NULL;
		for (p_iptype = *ischeme->ptypes; *p_iptype; p_iptype++) {
			kptype_t *kptype = NULL;
			iptype_t *iptype = *p_iptype;

			kptype = kptype_from_iptype(iptype, error);
			if (!kptype) {
				retval = BOOL_FALSE; // Don't stop
				continue;
			}
			if (!kscheme_add_ptype(kscheme, kptype)) {
				// Search for PTYPE duplicates
				if (kscheme_find_ptype(kscheme,
					kptype_name(kptype))) {
					faux_error_sprintf(error,
						TAG": Can't add duplicate PTYPE "
						"\"%s\"", kptype_name(kptype));
				} else {
					faux_error_sprintf(error,
						TAG": Can't add PTYPE \"%s\"",
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
				if (!kview_parse(kview, iview, error)) {
					retval = BOOL_FALSE;
					continue;
				}
				if (!kview_nested_from_iview(kview, iview,
					error)) {
					retval = BOOL_FALSE;
					continue;
				}
				continue;
			}

			// New VIEW
			kview = kview_from_iview(iview, error);
			if (!kview) {
				retval = BOOL_FALSE;
				continue;
			}
			if (!kscheme_add_view(kscheme, kview)) {
				faux_error_sprintf(error,
					TAG": Can't add VIEW \"%s\"",
					kview_name(kview));
				kview_free(kview);
				retval = BOOL_FALSE;
				continue;
			}
		}
	}

	if (!retval)
		faux_error_sprintf(error, TAG": Illegal nested elements");

	return retval;
}


kscheme_t *kscheme_from_ischeme(ischeme_t *ischeme, faux_error_t *error)
{
	kscheme_t *kscheme = NULL;

	kscheme = kscheme_new();
	if (!kscheme) {
		faux_error_sprintf(error, TAG": Can't create object");
		return NULL;
	}

	// Parse nested elements
	if (!kscheme_nested_from_ischeme(kscheme, ischeme, error)) {
		kscheme_free(kscheme);
		return NULL;
	}

	return kscheme;
}
