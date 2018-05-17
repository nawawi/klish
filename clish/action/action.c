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
#include "lub/bintree.h"
#include "lub/string.h"

/*--------------------------------------------------------- */
static void clish_action_init(clish_action_t *this)
{
	this->script = NULL;
	this->builtin = NULL;
	this->shebang = NULL;
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

/*--------------------------------------------------------- */
void clish_action__set_script(clish_action_t *this, const char *script)
{
	if (this->script)
		lub_string_free(this->script);
	this->script = lub_string_dup(script);
}

CLISH_GET(action, const char *, script);

/*--------------------------------------------------------- */
void clish_action__set_builtin(clish_action_t *this, clish_sym_t *builtin)
{
	this->builtin = builtin;
}

CLISH_GET(action, clish_sym_t *, builtin);

/*--------------------------------------------------------- */
void clish_action__set_shebang(clish_action_t *this, const char *shebang)
{
	const char *prog = shebang;
	const char *prefix = "#!";

	if (this->shebang)
		lub_string_free(this->shebang);
	if (lub_string_nocasestr(shebang, prefix) == shebang)
		prog += strlen(prefix);
	this->shebang = lub_string_dup(prog);
}

//CLISH_GET(clish_action__get_script, clish_action_t, const char *, script);
/*--------------------------------------------------------- */
const char *clish_action__get_shebang(const clish_action_t *this)
{
	return this->shebang;
}
