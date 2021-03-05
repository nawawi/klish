#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <klish/khelper.h>
#include <klish/iparam.h>


char *iparam_to_text(const iparam_t *iparam, int level)
{
	char *str = NULL;
	char *tmp = NULL;

	tmp = faux_str_sprintf("%*cPARAM {\n", level, ' ');
	faux_str_cat(&str, tmp);
	faux_str_free(tmp);

	attr2ctext(&str, "name", iparam->name, level + 1);
	attr2ctext(&str, "help", iparam->help, level + 1);
	attr2ctext(&str, "ptype", iparam->ptype, level + 1);

	// PARAM list
	if (iparam->params) {
		iparam_t **p_iparam = NULL;

		tmp = faux_str_sprintf("\n%*cPARAM_LIST\n\n", level + 1, ' ');
		faux_str_cat(&str, tmp);
		faux_str_free(tmp);

		for (p_iparam = *iparam->params; *p_iparam; p_iparam++) {
			iparam_t *niparam = *p_iparam;

			tmp = iparam_to_text(niparam, level + 2);
			faux_str_cat(&str, tmp);
			faux_str_free(tmp);
		}

		tmp = faux_str_sprintf("%*cEND_PARAM_LIST,\n", level + 1, ' ');
		faux_str_cat(&str, tmp);
		faux_str_free(tmp);
	}

	tmp = faux_str_sprintf("%*c},\n\n", level, ' ');
	faux_str_cat(&str, tmp);
	faux_str_free(tmp);

	return str;
}
