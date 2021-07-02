#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <faux/list.h>
#include <klish/khelper.h>
#include <klish/kplugin.h>
#include <klish/kentry.h>
#include <klish/kscheme.h>
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
			if (!kscheme_add_plugins(kscheme, kplugin)) {
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

	// ENTRY list
	// ENTRYs can be duplicate. Duplicated ENTRY will add nested
	// elements to existent ENTRY. Also it can overwrite ENTRY attributes.
	// So there is no special rule which attribute value will be "on top".
	// It's a random. Technically later ENTRYs will rewrite previous
	// values.
	if (ischeme->entrys) {
		ientry_t **p_ientry = NULL;
		for (p_ientry = *ischeme->entrys; *p_ientry; p_ientry++) {
			kentry_t *nkentry = NULL;
			ientry_t *nientry = *p_ientry;
			const char *entry_name = nientry->name;

			if (entry_name)
				nkentry = kscheme_find_entry(kscheme, entry_name);

			// ENTRY already exists
			if (nkentry) {
				if (!ientry_parse(nientry, nkentry, error)) {
					retval = BOOL_FALSE;
					continue;
				}
				if (!ientry_parse_nested(nientry, nkentry,
					error)) {
					retval = BOOL_FALSE;
					continue;
				}
				continue;
			}

			// New ENTRY
			nkentry = ientry_load(nientry, error);
			if (!nkentry) {
				retval = BOOL_FALSE;
				continue;
			}
			kentry_set_parent(nkentry, NULL); // Set empty parent entry
			if (!kscheme_add_entrys(kscheme, nkentry)) {
				faux_error_sprintf(error,
					TAG": Can't add ENTRY \"%s\"",
					kentry_name(nkentry));
				kentry_free(nkentry);
				retval = BOOL_FALSE;
				continue;
			}
		}
	}

	if (!retval)
		faux_error_sprintf(error, TAG": Illegal nested elements");

	return retval;
}


bool_t ischeme_load(const ischeme_t *ischeme, kscheme_t *kscheme,
	faux_error_t *error)
{
	assert(kscheme);
	if (!kscheme) {
		faux_error_sprintf(error, TAG": Internal error");
		return BOOL_FALSE;
	}

	// Parse nested elements
	return ischeme_parse_nested(ischeme, kscheme, error);
}


char *ischeme_deploy(const kscheme_t *scheme, int level)
{
	char *str = NULL;
	char *tmp = NULL;
	kscheme_plugins_node_t *plugins_iter = NULL;
	kscheme_entrys_node_t *entrys_iter = NULL;

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

	// ENTRY list
	entrys_iter = kscheme_entrys_iter(scheme);
	if (entrys_iter) {
		kentry_t *entry = NULL;

		tmp = faux_str_sprintf("\n%*cENTRY_LIST\n\n", level, ' ');
		faux_str_cat(&str, tmp);
		faux_str_free(tmp);

		while ((entry = kscheme_entrys_each(&entrys_iter))) {
			tmp = ientry_deploy(entry, level + 2);
			faux_str_cat(&str, tmp);
			faux_str_free(tmp);
		}

		tmp = faux_str_sprintf("%*cEND_ENTRY_LIST,\n", level + 1, ' ');
		faux_str_cat(&str, tmp);
		faux_str_free(tmp);
	}

	tmp = faux_str_sprintf("};\n");
	faux_str_cat(&str, tmp);
	faux_str_free(tmp);

	return str;
}
