#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <faux/conv.h>
#include <klish/khelper.h>
#include <klish/kptype.h>
#include <klish/kaction.h>


char *iptype_to_text(const iptype_t *iptype, int level)
{
	char *str = NULL;
	char *tmp = NULL;

	tmp = faux_str_sprintf("%*cPTYPE = {\n", level, ' ');
	faux_str_cat(&str, tmp);
	faux_str_free(tmp);

	attr2ctext(&str, "name", iptype->name, level + 1);
	attr2ctext(&str, "help", iptype->help, level + 1);

	// ACTION list
	if (iptype->actions) {
		iaction_t **p_iaction = NULL;

		tmp = faux_str_sprintf("\n%*cACTION_LIST\n\n", level + 1, ' ');
		faux_str_cat(&str, tmp);
		faux_str_free(tmp);

		for (p_iaction = *iptype->actions; *p_iaction; p_iaction++) {
			iaction_t *iaction = *p_iaction;

			tmp = iaction_to_text(iaction, level + 2);
			faux_str_cat(&str, tmp);
			faux_str_free(tmp);
		}

		tmp = faux_str_sprintf("%*cEND_ACTION_LIST,\n", level + 1, ' ');
		faux_str_cat(&str, tmp);
		faux_str_free(tmp);
	}

	tmp = faux_str_sprintf("%*c},\n\n", level, ' ');
	faux_str_cat(&str, tmp);
	faux_str_free(tmp);

	return str;
}
