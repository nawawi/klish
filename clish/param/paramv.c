/*
 * paramv.c
 *
 */
#include "private.h"
#include "lub/string.h"
#include "clish/types.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

	if (this)
		clish_paramv_init(this);
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
	if (tmp) {
		this->paramv = tmp;
		/* insert reference to the parameter */
		this->paramv[this->paramc++] = param;
	}
}

/*--------------------------------------------------------- */
int clish_paramv_remove(clish_paramv_t *this, unsigned int index)
{
	size_t new_size;
	clish_param_t **tmp;
	clish_param_t **dst, **src;
	size_t n;

	if (this->paramc < 1)
		return -1;
	if (index >= this->paramc)
		return -1;

	new_size = ((this->paramc - 1) * sizeof(clish_param_t *));
	dst = this->paramv + index;
	src = dst + 1;
	n = this->paramc - index - 1;
	if (n)
		memmove(dst, src, n * sizeof(clish_param_t *));
	/* Resize the parameter vector */
	if (new_size) {
		tmp = realloc(this->paramv, new_size);
		if (!tmp)
			return -1;
		this->paramv = tmp;
	} else {
		free(this->paramv);
		this->paramv = NULL;
	}
	this->paramc--;

	return 0;
}

/*--------------------------------------------------------- */
clish_param_t *clish_paramv__get_param(const clish_paramv_t * this,
	unsigned int index)
{
	clish_param_t *result = NULL;

	if (index < this->paramc)
		result = this->paramv[index];
	return result;
}

/*--------------------------------------------------------- */
clish_param_t *clish_paramv_find_param(const clish_paramv_t * this,
	const char *name)
{
	clish_param_t *res = NULL;
	unsigned int i;

	for (i = 0; i < this->paramc; i++) {
		if (!strcmp(clish_param__get_name(this->paramv[i]), name))
			return this->paramv[i];
		if ((res = clish_paramv_find_param(
			clish_param__get_paramv(this->paramv[i]), name)))
			return res;
	}

	return res;
}

/*--------------------------------------------------------- */
const char *clish_paramv_find_default(const clish_paramv_t * this,
	const char *name)
{
	clish_param_t *res = clish_paramv_find_param(this, name);

	if (res)
		return clish_param__get_defval(res);

	return NULL;
}

/*--------------------------------------------------------- */
unsigned int clish_paramv__get_count(const clish_paramv_t * this)
{
	return this->paramc;
}

