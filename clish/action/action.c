/*
 * action.c
 *
 * This file provides the implementation of a action definition
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "private.h"
#include "lub/string.h"

/*--------------------------------------------------------- */
static void clish_action_init(clish_action_t *this)
{
	this->script = NULL;
	this->builtin = NULL;
	this->shebang = NULL;
	this->lock = BOOL_TRUE;
	this->interrupt = BOOL_FALSE;
}

/*--------------------------------------------------------- */
static void clish_action_fini(clish_action_t *this)
{
	lub_string_free(this->script);
	lub_string_free(this->shebang);
}

/*--------------------------------------------------------- */
clish_action_t *clish_action_new(void)
{
	clish_action_t *this = malloc(sizeof(clish_action_t));

	if (this)
		clish_action_init(this);

	return this;
}

/*--------------------------------------------------------- */
void clish_action_delete(clish_action_t *this)
{
	clish_action_fini(this);
	free(this);
}

CLISH_SET_STR(action, script);
CLISH_GET_STR(action, script);
CLISH_SET(action, const clish_sym_t *, builtin);
CLISH_GET(action, const clish_sym_t *, builtin);
CLISH_SET(action, bool_t, lock);
CLISH_GET(action, bool_t, lock);
CLISH_SET(action, bool_t, interrupt);
CLISH_GET(action, bool_t, interrupt);
CLISH_SET(action, bool_t, interactive);
CLISH_GET(action, bool_t, interactive);

_CLISH_SET_STR(action, shebang)
{
	const char *prog = val;
	const char *prefix = "#!";

	lub_string_free(inst->shebang);
	if (lub_string_nocasestr(val, prefix) == val)
		prog += strlen(prefix);
	inst->shebang = lub_string_dup(prog);
}

CLISH_GET_STR(action, shebang);
