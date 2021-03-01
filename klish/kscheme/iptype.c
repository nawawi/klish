#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <klish/khelper.h>
#include <klish/kptype.h>
#include <klish/kaction.h>

#define ISCHEME_TAB " "

char *iptype_to_text(const iptype_t *iptype, int level)
{
	char *str = NULL;
	char *tmp = NULL;

	tmp = faux_str_sprintf("%*cPTYPE = {\n", level, ' ');
	faux_str_cat(&str, tmp);
	faux_str_free(tmp);


/*
	// PTYPE list
	if (iptype->ptypes) {
		iptype_t **p_iptype = NULL;

		tmp = faux_str_sprintf("\n%*sPTYPE_LIST\n\n", level + 1, ISCHEME_TAB);
		faux_str_cat(&str, tmp);
		faux_str_free(tmp);

		for (p_iptype = *iptype->ptypes; *p_iptype; p_iptype++) {
			iptype_t *iptype = *p_iptype;

			tmp = iptype_to_text(iptype, level + 2);
			faux_str_cat(&str, tmp);
			faux_str_free(tmp);
		}

		tmp = faux_str_sprintf("\n%*sEND_PTYPE_LIST,\n", level + 1, ISCHEME_TAB);
		faux_str_cat(&str, tmp);
		faux_str_free(tmp);
	}
*/
	tmp = faux_str_sprintf("%*c},\n\n", level, ' ');
	faux_str_cat(&str, tmp);
	faux_str_free(tmp);
iptype = iptype;
	return str;
}

