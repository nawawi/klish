/*
 * param.c
 *
 * This file provides the implementation of the "param" class
 */
#include "private.h"
#include "lub/string.h"
#include "clish/types.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*--------------------------------------------------------- */
static void clish_param_init(clish_param_t *this, const char *name,
	const char *text, const char *ptype_name)
{
	this->name = lub_string_dup(name);
	this->text = lub_string_dup(text);
	this->ptype_name = lub_string_dup(ptype_name);

	/* Set up defaults */
	this->ptype = NULL;
	this->defval = NULL;
	this->mode = CLISH_PARAM_COMMON;
	this->optional = BOOL_FALSE;
	this->order = BOOL_FALSE;
	this->value = NULL;
	this->hidden = BOOL_FALSE;
	this->test = NULL;
	this->completion = NULL;
	this->access = NULL;

	this->paramv = clish_paramv_new();
}

/*--------------------------------------------------------- */
static void clish_param_fini(clish_param_t * this)
{
	/* deallocate the memory for this instance */
	lub_string_free(this->defval);
	lub_string_free(this->name);
	lub_string_free(this->text);
	lub_string_free(this->ptype_name);
	lub_string_free(this->value);
	lub_string_free(this->test);
	lub_string_free(this->completion);
	lub_string_free(this->access);

	clish_paramv_delete(this->paramv);
}

/*--------------------------------------------------------- */
clish_param_t *clish_param_new(const char *name, const char *text,
	const char *ptype_name)
{
	clish_param_t *this = malloc(sizeof(clish_param_t));

	if (this)
		clish_param_init(this, name, text, ptype_name);
	return this;
}

/*--------------------------------------------------------- */
void clish_param_delete(clish_param_t * this)
{
	clish_param_fini(this);
	free(this);
}

/*--------------------------------------------------------- */
void clish_param_insert_param(clish_param_t * this, clish_param_t * param)
{
	return clish_paramv_insert(this->paramv, param);
}

/*--------------------------------------------------------- */
char *clish_param_validate(const clish_param_t * this, const char *text)
{
	if (CLISH_PARAM_SUBCOMMAND == clish_param__get_mode(this)) {
		if (lub_string_nocasecmp(clish_param__get_value(this), text))
			return NULL;
	}
	return clish_ptype_translate(this->ptype, text);
}

/*--------------------------------------------------------- */
void clish_param_help(const clish_param_t * this, clish_help_t *help)
{
	const char *range = clish_ptype__get_range(this->ptype);
	const char *name;
	char *str = NULL;

	if (CLISH_PARAM_SWITCH == clish_param__get_mode(this)) {
		unsigned rec_paramc = clish_param__get_param_count(this);
		clish_param_t *cparam;
		unsigned i;

		for (i = 0; i < rec_paramc; i++) {
			cparam = clish_param__get_param(this, i);
			if (!cparam)
				break;
			clish_param_help(cparam, help);
		}
		return;
	}

	if (CLISH_PARAM_SUBCOMMAND == clish_param__get_mode(this))
		name = clish_param__get_value(this);
	else
		if (!(name = clish_ptype__get_text(this->ptype)))
			name = clish_ptype__get_name(this->ptype);

	lub_string_cat(&str, this->text);
	if (range) {
		lub_string_cat(&str, " (");
		lub_string_cat(&str, range);
		lub_string_cat(&str, ")");
	}
	lub_argv_add(help->name, name);
	lub_argv_add(help->help, str);
	lub_string_free(str);
	lub_argv_add(help->detail, NULL);
}

/*--------------------------------------------------------- */
void clish_param_help_arrow(const clish_param_t * this, size_t offset)
{
	fprintf(stderr, "%*c\n", (int)offset, '^');

	this = this; /* Happy compiler */
}

CLISH_SET_STR(param, ptype_name);
CLISH_GET_STR(param, ptype_name);
CLISH_SET_STR(param, access);
CLISH_GET_STR(param, access);
CLISH_GET_STR(param, name);
CLISH_GET_STR(param, text);
CLISH_SET_STR_ONCE(param, value);
CLISH_SET(param, clish_ptype_t *, ptype);
CLISH_GET(param, clish_ptype_t *, ptype);
CLISH_SET_STR_ONCE(param, defval);
CLISH_GET_STR(param, defval);
CLISH_SET_STR_ONCE(param, test);
CLISH_GET_STR(param, test);
CLISH_SET_STR_ONCE(param, completion);
CLISH_GET_STR(param, completion);
CLISH_SET(param, clish_param_mode_e, mode);
CLISH_GET(param, clish_param_mode_e, mode);
CLISH_GET(param, clish_paramv_t *, paramv);
CLISH_SET(param, bool_t, optional);
CLISH_GET(param, bool_t, optional);
CLISH_SET(param, bool_t, order);
CLISH_GET(param, bool_t, order);
CLISH_SET(param, bool_t, hidden);
CLISH_GET(param, bool_t, hidden);

/*--------------------------------------------------------- */
_CLISH_GET_STR(param, value)
{
	assert(inst);
	if (inst->value)
		return inst->value;
	return inst->name;
}

/*--------------------------------------------------------- */
_CLISH_GET_STR(param, range)
{
	assert(inst);
	return clish_ptype__get_range(inst->ptype);
}

/*--------------------------------------------------------- */
clish_param_t *clish_param__get_param(const clish_param_t * this,
	unsigned int index)
{
	assert(this);
	return clish_paramv__get_param(this->paramv, index);
}

/*--------------------------------------------------------- */
_CLISH_GET(param, unsigned int, param_count)
{
	assert(inst);
	return clish_paramv__get_count(inst->paramv);
}
