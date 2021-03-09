#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <faux/list.h>
#include <klish/khelper.h>
#include <klish/kplugin.h>
#include <klish/kptype.h>
#include <klish/kview.h>
#include <klish/kscheme.h>
#include <klish/iview.h>
#include <klish/ischeme.h>

#define TAG "SCHEME"


bool_t ischeme_parse_nested(const ischeme_t *ischeme, kscheme_t *kscheme,
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

			kplugin = iplugin_load(iplugin, error);
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

			kptype = iptype_load(iptype, error);
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
				if (!iview_parse(iview, kview, error)) {
					retval = BOOL_FALSE;
					continue;
				}
				if (!iview_parse_nested(iview, kview,
					error)) {
					retval = BOOL_FALSE;
					continue;
				}
				continue;
			}

			// New VIEW
			kview = iview_load(iview, error);
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


kscheme_t *ischeme_load(const ischeme_t *ischeme, faux_error_t *error)
{
	kscheme_t *kscheme = NULL;

	kscheme = kscheme_new();
	if (!kscheme) {
		faux_error_sprintf(error, TAG": Can't create object");
		return NULL;
	}

	// Parse nested elements
	if (!ischeme_parse_nested(ischeme, kscheme, error)) {
		kscheme_free(kscheme);
		return NULL;
	}

	return kscheme;
}


char *ischeme_deploy(const kscheme_t *scheme, int level)
{
	char *str = NULL;
	char *tmp = NULL;
	kscheme_plugins_node_t *plugins_iter = NULL;
	kscheme_ptypes_node_t *ptypes_iter = NULL;
	kscheme_views_node_t *views_iter = NULL;

	tmp = faux_str_sprintf("ischeme_t sch = {\n");
	faux_str_cat(&str, tmp);
	faux_str_free(tmp);

	// PLUGIN list
	plugins_iter = kscheme_plugins_iter(scheme);
	if (plugins_iter) {
		kplugin_t *plugin = NULL;

		tmp = faux_str_sprintf("\n%*cPLUGIN_LIST\n\n", level, ' ');
		faux_str_cat(&str, tmp);
		faux_str_free(tmp);

		while ((plugin = kscheme_plugins_each(&plugins_iter))) {
			tmp = iplugin_deploy(plugin, level + 2);
			faux_str_cat(&str, tmp);
			faux_str_free(tmp);
		}

		tmp = faux_str_sprintf("%*cEND_PLUGIN_LIST,\n", level + 1, ' ');
		faux_str_cat(&str, tmp);
		faux_str_free(tmp);
	}

	// PTYPE list
	ptypes_iter = kscheme_ptypes_iter(scheme);
	if (ptypes_iter) {
		kptype_t *ptype = NULL;

		tmp = faux_str_sprintf("\n%*cPTYPE_LIST\n\n", level, ' ');
		faux_str_cat(&str, tmp);
		faux_str_free(tmp);

		while ((ptype = kscheme_ptypes_each(&ptypes_iter))) {
			tmp = iptype_deploy(ptype, level + 2);
			faux_str_cat(&str, tmp);
			faux_str_free(tmp);
		}

		tmp = faux_str_sprintf("%*cEND_PTYPE_LIST,\n", level + 1, ' ');
		faux_str_cat(&str, tmp);
		faux_str_free(tmp);
	}

	// VIEW list
	views_iter = kscheme_views_iter(scheme);
	if (views_iter) {
		kview_t *view = NULL;

		tmp = faux_str_sprintf("\n%*cVIEW_LIST\n\n", level + 1, ' ');
		faux_str_cat(&str, tmp);
		faux_str_free(tmp);

		while ((view = kscheme_views_each(&views_iter))) {
			tmp = iview_deploy(view, level + 2);
			faux_str_cat(&str, tmp);
			faux_str_free(tmp);
		}

		tmp = faux_str_sprintf("%*cEND_VIEW_LIST,\n", level + 1, ' ');
		faux_str_cat(&str, tmp);
		faux_str_free(tmp);
	}

	tmp = faux_str_sprintf("};\n");
	faux_str_cat(&str, tmp);
	faux_str_free(tmp);

	return str;
}
