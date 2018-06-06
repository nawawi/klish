/*
 * plugin.c
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "private.h"
#include "lub/string.h"
#include "lub/list.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif

/*--------------------------------------------------------- */
int clish_sym_compare(const void *first, const void *second)
{
	const clish_sym_t *f = (const clish_sym_t *)first;
	const clish_sym_t *s = (const clish_sym_t *)second;

	return strcmp(f->name, s->name);
}

/*--------------------------------------------------------- */
clish_sym_t *clish_sym_new(const char *name, void *func, int type)
{
	clish_sym_t *this;

	this = malloc(sizeof(*this));
	this->name = lub_string_dup(name);
	this->func = func;
	this->type = type;
	this->api = CLISH_SYM_API_SIMPLE;
	this->permanent = BOOL_FALSE;

	return this;
}

/*--------------------------------------------------------- */
void clish_sym_free(void *data)
{
	clish_sym_t *this = (clish_sym_t *)data;
	if (!data)
		return;
	lub_string_free(this->name);
	free(this);
}

/*--------------------------------------------------------- */
int clish_sym_clone(clish_sym_t *dst, clish_sym_t *src)
{
	char *name;

	if (!dst || !src)
		return -1;
	name = dst->name;
	*dst = *src;
	dst->name = name;

	return 0;
}

CLISH_SET(sym, const void *, func);
CLISH_GET(sym, const void *, func);
CLISH_SET(sym, bool_t, permanent);
CLISH_GET(sym, bool_t, permanent);
CLISH_SET_STR(sym, name);
CLISH_GET_STR(sym, name);
CLISH_SET(sym, clish_plugin_t *, plugin);
CLISH_GET(sym, clish_plugin_t *, plugin);
CLISH_SET(sym, int, type);
CLISH_GET(sym, int, type);
CLISH_SET(sym, clish_sym_api_e, api);
CLISH_GET(sym, clish_sym_api_e, api);
