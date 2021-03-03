#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <klish/khelper.h>
#include <klish/kview.h>
#include <klish/kptype.h>
#include <klish/kscheme.h>


char *ischeme_to_text(const ischeme_t *ischeme, int level)
{
	char *str = NULL;
	char *tmp = NULL;

	tmp = faux_str_sprintf("ischeme_t sch = {\n");
	faux_str_cat(&str, tmp);
	faux_str_free(tmp);

	// PLUGIN list
	if (ischeme->plugins) {
		iplugin_t **p_iplugin = NULL;

		tmp = faux_str_sprintf("\n%*cPLUGIN_LIST\n\n", level, ' ');
		faux_str_cat(&str, tmp);
		faux_str_free(tmp);

		for (p_iplugin = *ischeme->plugins; *p_iplugin; p_iplugin++) {
			iplugin_t *iplugin = *p_iplugin;

			tmp = iplugin_to_text(iplugin, level + 2);
			faux_str_cat(&str, tmp);
			faux_str_free(tmp);
		}

		tmp = faux_str_sprintf("%*cEND_PLUGIN_LIST,\n", level + 1, ' ');
		faux_str_cat(&str, tmp);
		faux_str_free(tmp);
	}

	// PTYPE list
	if (ischeme->ptypes) {
		iptype_t **p_iptype = NULL;

		tmp = faux_str_sprintf("\n%*cPTYPE_LIST\n\n", level, ' ');
		faux_str_cat(&str, tmp);
		faux_str_free(tmp);

		for (p_iptype = *ischeme->ptypes; *p_iptype; p_iptype++) {
			iptype_t *iptype = *p_iptype;

			tmp = iptype_to_text(iptype, level + 2);
			faux_str_cat(&str, tmp);
			faux_str_free(tmp);
		}

		tmp = faux_str_sprintf("%*cEND_PTYPE_LIST,\n", level + 1, ' ');
		faux_str_cat(&str, tmp);
		faux_str_free(tmp);
	}

	// VIEW list
	if (ischeme->views) {
		iview_t **p_iview = NULL;

		tmp = faux_str_sprintf("\n%*cVIEW_LIST\n\n", level + 1, ' ');
		faux_str_cat(&str, tmp);
		faux_str_free(tmp);

		for (p_iview = *ischeme->views; *p_iview; p_iview++) {
			iview_t *iview = *p_iview;

			tmp = iview_to_text(iview, level + 2);
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
