#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <faux/conv.h>
#include <klish/khelper.h>
#include <klish/kptype.h>
#include <klish/kaction.h>


bool_t attr2ctext(char **dst, const char *field, const char *value, int level)
{
	char *tmp = NULL;
	char *esc = NULL;

	if (!field) // Error
		return BOOL_FALSE;
	if (faux_str_is_empty(value)) // Not error. Just empty field.
		return BOOL_TRUE;

	esc = faux_str_c_esc(value);
	if (!esc)
		return BOOL_FALSE;
	tmp = faux_str_sprintf("%*c.%s = \"%s\",\n",
		level, ' ', field, esc);
	faux_str_free(esc);
	faux_str_cat(dst, tmp);
	faux_str_free(tmp);

	return BOOL_TRUE;
}
