#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <faux/conv.h>
#include <klish/khelper.h>
#include <klish/kplugin.h>


char *iplugin_to_text(const iplugin_t *iplugin, int level)
{
	char *str = NULL;
	char *tmp = NULL;

	tmp = faux_str_sprintf("%*cPLUGIN {\n", level, ' ');
	faux_str_cat(&str, tmp);
	faux_str_free(tmp);

	attr2ctext(&str, "name", iplugin->name, level + 1);
	attr2ctext(&str, "file", iplugin->file, level + 1);
	attr2ctext(&str, "global", iplugin->global, level + 1);

	tmp = faux_str_sprintf("%*c},\n\n", level, ' ');
	faux_str_cat(&str, tmp);
	faux_str_free(tmp);

	return str;
}
