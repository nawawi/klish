#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <faux/conv.h>
#include <klish/iaction.h>

char *iaction_to_text(const iaction_t *iaction, int level)
{
	char *str = NULL;
	char *tmp = NULL;

	tmp = faux_str_sprintf("%*cACTION {\n", level, ' ');
	faux_str_cat(&str, tmp);
	faux_str_free(tmp);

	attr2ctext(&str, "sym", iaction->sym, level + 1);
	attr2ctext(&str, "lock", iaction->lock, level + 1);
	attr2ctext(&str, "interrupt", iaction->interrupt, level + 1);
	attr2ctext(&str, "interactive", iaction->interactive, level + 1);
	attr2ctext(&str, "exec_on", iaction->exec_on, level + 1);
	attr2ctext(&str, "update_retcode", iaction->update_retcode, level + 1);
	attr2ctext(&str, "script", iaction->script, level + 1);

	tmp = faux_str_sprintf("%*c},\n\n", level, ' ');
	faux_str_cat(&str, tmp);
	faux_str_free(tmp);

	return str;
}
