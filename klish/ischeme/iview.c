#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <klish/khelper.h>
#include <klish/iview.h>


char *iview_to_text(const iview_t *iview, int level)
{
	char *str = NULL;
	char *tmp = NULL;

	tmp = faux_str_sprintf("%*cVIEW {\n", level, ' ');
	faux_str_cat(&str, tmp);
	faux_str_free(tmp);

	attr2ctext(&str, "name", iview->name, level + 1);

	// COMMAND list
	if (iview->commands) {
		icommand_t **p_icommand = NULL;

		tmp = faux_str_sprintf("\n%*cCOMMAND_LIST\n\n", level + 1, ' ');
		faux_str_cat(&str, tmp);
		faux_str_free(tmp);

		for (p_icommand = *iview->commands; *p_icommand; p_icommand++) {
			icommand_t *icommand = *p_icommand;

			tmp = icommand_to_text(icommand, level + 2);
			faux_str_cat(&str, tmp);
			faux_str_free(tmp);
		}

		tmp = faux_str_sprintf("%*cEND_COMMAND_LIST,\n", level + 1, ' ');
		faux_str_cat(&str, tmp);
		faux_str_free(tmp);
	}

	tmp = faux_str_sprintf("%*c},\n\n", level, ' ');
	faux_str_cat(&str, tmp);
	faux_str_free(tmp);

	return str;
}
