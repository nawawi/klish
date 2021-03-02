#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <faux/conv.h>
#include <klish/khelper.h>
#include <klish/kcommand.h>
#include <klish/kaction.h>


char *icommand_to_text(const icommand_t *icommand, int level)
{
	char *str = NULL;
	char *tmp = NULL;

	tmp = faux_str_sprintf("%*cCOMMAND {\n", level, ' ');
	faux_str_cat(&str, tmp);
	faux_str_free(tmp);

	attr2ctext(&str, "name", icommand->name, level + 1);
	attr2ctext(&str, "help", icommand->help, level + 1);

	// PARAM list
	if (icommand->params) {
		iparam_t **p_iparam = NULL;

		tmp = faux_str_sprintf("\n%*cPARAM_LIST\n\n", level + 1, ' ');
		faux_str_cat(&str, tmp);
		faux_str_free(tmp);

		for (p_iparam = *icommand->params; *p_iparam; p_iparam++) {
			iparam_t *iparam = *p_iparam;

			tmp = iparam_to_text(iparam, level + 2);
			faux_str_cat(&str, tmp);
			faux_str_free(tmp);
		}

		tmp = faux_str_sprintf("%*cEND_PARAM_LIST,\n", level + 1, ' ');
		faux_str_cat(&str, tmp);
		faux_str_free(tmp);
	}

	// ACTION list
	if (icommand->actions) {
		iaction_t **p_iaction = NULL;

		tmp = faux_str_sprintf("\n%*cACTION_LIST\n\n", level + 1, ' ');
		faux_str_cat(&str, tmp);
		faux_str_free(tmp);

		for (p_iaction = *icommand->actions; *p_iaction; p_iaction++) {
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
