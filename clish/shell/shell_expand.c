/*
 * shell_expand.c
 */
#include <stdlib.h>
#include <assert.h>

#include "lub/string.h"
#include "private.h"

/*----------------------------------------------------------- */
char * clish_shell__expand_text(const clish_shell_t *this,
	clish_command_t *cmd, clish_pargv_t *pargv, const char *text)
{
	assert(this);
	if (!text)
		return NULL;
	return clish_variable_expand(text, this->viewid, cmd, pargv);
}

/*----------------------------------------------------------- */
char * clish_shell__expand_variable(const clish_shell_t *this,
	clish_command_t *cmd, clish_pargv_t *pargv, const char *var)
{
	assert(this);
	if (!var)
		return NULL;
	return clish_variable__get_value(var, this->viewid, cmd, pargv);
}

/*----------------------------------------------------------- */
