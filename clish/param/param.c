/*
 * param.c
 *
 * This file provides the implementation of the "param" class  
 */
#include "private.h"
#include "lub/string.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*---------------------------------------------------------
 * PRIVATE METHODS
 *--------------------------------------------------------- */
static void
clish_param_init(clish_param_t * this,
		 const char *name, const char *text, clish_ptype_t * ptype)
{

	/* initialise the help part */
	this->name = lub_string_dup(name);
	this->text = lub_string_dup(text);
	this->ptype = ptype;

	/* set up defaults */
	this->defval = NULL;
	this->mode = CLISH_PARAM_COMMON;
	this->optional = BOOL_FALSE;

	this->paramv = clish_paramv_new();
}

/*--------------------------------------------------------- */
static void clish_param_fini(clish_param_t * this)
{
	unsigned i;

	/* deallocate the memory for this instance */
	lub_string_free(this->defval);
	this->defval = NULL;
	lub_string_free(this->name);
	this->name = NULL;
	lub_string_free(this->text);
	this->text = NULL;

	clish_paramv_delete(this->paramv);
}

/*---------------------------------------------------------
 * PUBLIC META FUNCTIONS
 *--------------------------------------------------------- */
clish_param_t *clish_param_new(const char *name,
			       const char *text, clish_ptype_t * ptype)
{
	clish_param_t *this = malloc(sizeof(clish_param_t));

	if (this) {
		clish_param_init(this, name, text, ptype);
	}
	return this;
}

/*---------------------------------------------------------
 * PUBLIC METHODS
 *--------------------------------------------------------- */
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

/*---------------------------------------------------------
 * PUBLIC ATTRIBUTES
 *--------------------------------------------------------- */
const char *clish_param__get_name(const clish_param_t * this)
{
	if (!this)
		return NULL;
	return this->name;
}

/*--------------------------------------------------------- */
const char *clish_param__get_text(const clish_param_t * this)
{
	return this->text;
}

/*--------------------------------------------------------- */
const char *clish_param__get_range(const clish_param_t * this)
{
	return clish_ptype__get_range(this->ptype);
}

/*--------------------------------------------------------- */
clish_ptype_t *clish_param__get_ptype(const clish_param_t * this)
{
	return this->ptype;
}

/*--------------------------------------------------------- */
void clish_param__set_default(clish_param_t * this, const char *defval)
{
	assert(NULL == this->defval);
	this->defval = lub_string_dup(defval);
}

/*--------------------------------------------------------- */
const char *clish_param__get_default(const clish_param_t * this)
{
	return this->defval;
}

/*--------------------------------------------------------- */
void clish_param__set_mode(clish_param_t * this, clish_param_mode_e mode)
{
	assert(this);
	this->mode = mode;
}

/*--------------------------------------------------------- */
clish_param_mode_e clish_param__get_mode(const clish_param_t * this)
{
	return this->mode;
}

/*--------------------------------------------------------- */
char *clish_param_validate(const clish_param_t * this, const char *text)
{
	if (CLISH_PARAM_SUBCOMMAND == clish_param__get_mode(this)) {
		if (lub_string_nocasecmp(clish_param__get_name(this), text))
			return NULL;
	}
	return clish_ptype_translate(this->ptype, text);
}

/*--------------------------------------------------------- */
void clish_param_help(const clish_param_t * this, size_t offset)
{
	const char *range = clish_ptype__get_range(this->ptype);
	const char *name;

	if (CLISH_PARAM_SWITCH == clish_param__get_mode(this)) {
		unsigned rec_paramc = clish_param__get_param_count(this);
		clish_param_t *cparam;
		unsigned i;

		for (i = 0; i < rec_paramc; i++) {
			cparam = clish_param__get_param(this, i);
			if (!cparam)
				break;
			clish_param_help(cparam, offset);
		}
		return;
	}

	if (CLISH_PARAM_SUBCOMMAND == clish_param__get_mode(this))
		name = this->name;
	else
		name = clish_ptype__get_text(this->ptype);

	printf("%s %*c%s", name, (int)(offset - strlen(name)), ' ', this->text);
	if (NULL != range) {
		printf(" (%s)", range);
	}
	printf("\n");
}

/*--------------------------------------------------------- */
void clish_param_help_arrow(const clish_param_t * this, size_t offset)
{
	printf("%*c\n", (int)offset, '^');
}

/*--------------------------------------------------------- */
clish_param_t *clish_param__get_param(const clish_param_t * this,
				      unsigned index)
{
	return clish_paramv__get_param(this->paramv, index);
}

/*--------------------------------------------------------- */
clish_paramv_t *clish_param__get_paramv(clish_param_t * this)
{
	return this->paramv;
}

/*--------------------------------------------------------- */
const unsigned clish_param__get_param_count(const clish_param_t * this)
{
	return clish_paramv__get_count(this->paramv);
}

/*--------------------------------------------------------- */
void clish_param__set_optional(clish_param_t * this, bool_t optional)
{
	this->optional = optional;
}

/*--------------------------------------------------------- */
bool_t clish_param__get_optional(const clish_param_t * this)
{
	return this->optional;
}

/*--------------------------------------------------------- */

/* paramv methods */

/*--------------------------------------------------------- */
static void clish_paramv_init(clish_paramv_t * this)
{
	this->paramc = 0;
	this->paramv = NULL;
}

/*--------------------------------------------------------- */
static void clish_paramv_fini(clish_paramv_t * this)
{
	unsigned i;

	/* finalize each of the parameter instances */
	for (i = 0; i < this->paramc; i++) {
		clish_param_delete(this->paramv[i]);
	}
	/* free the parameter vector */
	free(this->paramv);
	this->paramc = 0;
}

/*--------------------------------------------------------- */
clish_paramv_t *clish_paramv_new(void)
{
	clish_paramv_t *this = malloc(sizeof(clish_paramv_t));

	if (this) {
		clish_paramv_init(this);
	}
	return this;
}

/*--------------------------------------------------------- */
void clish_paramv_delete(clish_paramv_t * this)
{
	clish_paramv_fini(this);
	free(this);
}

/*--------------------------------------------------------- */
void clish_paramv_insert(clish_paramv_t * this, clish_param_t * param)
{
	size_t new_size = ((this->paramc + 1) * sizeof(clish_param_t *));
	clish_param_t **tmp;

	/* resize the parameter vector */
	tmp = realloc(this->paramv, new_size);
	if (NULL != tmp) {
		this->paramv = tmp;
		/* insert reference to the parameter */
		this->paramv[this->paramc++] = param;
	}
}

/*--------------------------------------------------------- */
clish_param_t *clish_paramv__get_param(const clish_paramv_t * this,
				      unsigned index)
{
	clish_param_t *result = NULL;

	if (index < this->paramc) {
		result = this->paramv[index];
	}
	return result;
}

/*--------------------------------------------------------- */
const unsigned clish_paramv__get_count(const clish_paramv_t * this)
{
	return this->paramc;
}


