/*
 * shell_expand.c
 */
#include <stdlib.h>
#include <assert.h>

#include "lub/string.h"
#include "private.h"

/*--------------------------------------------------------- */
void clish_shell_insert_var(clish_shell_t *this, clish_var_t *var)
{
	(void)lub_bintree_insert(&this->var_tree, var);
}

/*--------------------------------------------------------- */
clish_var_t *clish_shell_find_var(clish_shell_t *this, const char *name)
{
	return lub_bintree_find(&this->var_tree, name);
}

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

/*--------------------------------------------------------- */
const char *clish_shell__get_viewid(const clish_shell_t *this)
{
	assert(this);
	return this->viewid;
}

/*----------------------------------------------------------- */
