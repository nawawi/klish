/*
 * plugin.c
 */
#include "private.h"
#include "lub/string.h"
#include "lub/list.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

/**********************************************************
 * SYM functions                                          *
 **********************************************************/

/*--------------------------------------------------------- */
static int clish_sym_compare(const void *first, const void *second)
{
	const clish_sym_t *f = (const clish_sym_t *)first;
	const clish_sym_t *s = (const clish_sym_t *)second;

	return strcmp(f->name, s->name);
}

/*--------------------------------------------------------- */
static clish_sym_t *clish_sym_new(const char *name, clish_plugin_fn_t *func)
{
	clish_sym_t *this;

	this = malloc(sizeof(*this));
	this->name = lub_string_dup(name);
	this->func = func;

	return this;
}

/*--------------------------------------------------------- */
static void clish_sym_free(clish_sym_t *this)
{
	if (!this)
		return;
	lub_string_free(this->name);
	free(this);
}

/**********************************************************
 * PLUGIN functions                                       *
 **********************************************************/

/*--------------------------------------------------------- */
clish_plugin_t *clish_plugin_new(const char *name, const char *file)
{
	clish_plugin_t *this;

	if (!file)
		return NULL;

	this = malloc(sizeof(*this));

	this->file = lub_string_dup(file);
	if (name)
		this->name = lub_string_dup(name);
	else
		this->name = NULL;
	/* Initialise the list of symbols */
	this->syms = lub_list_new(clish_sym_compare);

	return this;
}

/*--------------------------------------------------------- */
void clish_plugin_free(clish_plugin_t *this)
{
	lub_list_node_t *iter;

	if (!this)
		return;

	lub_string_free(this->file);
	lub_string_free(this->name);

	/* Free symbol list */
	while ((iter = lub_list__get_head(this->syms))) {
		/* Remove the symbol from the list */
		lub_list_del(this->syms, iter);
		/* Free the instance */
		clish_sym_free((clish_sym_t *)lub_list_node__get_data(iter));
		lub_list_node_free(iter);
	}
	lub_list_free(this->syms);

	free(this);
}

/*--------------------------------------------------------- */
int clish_plugin_sym(clish_plugin_t *this,
	clish_plugin_fn_t *func, const char *name)
{
	clish_sym_t *sym;

	if (!name || !func)
		return -1;

	if (!(sym = clish_sym_new(name, func)))
		return -1;
	lub_list_add(this->syms, sym);

	return 0;
}

/*--------------------------------------------------------- */
clish_plugin_fn_t *clish_plugin_dlsym(clish_plugin_t *this, const char *name)
{
	lub_list_node_t *iter;
	clish_sym_t *sym;

	/* Iterate elements */
	for(iter = lub_list__get_head(this->syms);
		iter; iter = lub_list_node__get_next(iter)) {
		sym = (clish_sym_t *)lub_list_node__get_data(iter);

	}

	return NULL;
}

/*--------------------------------------------------------- */
int clish_plugin_load(clish_plugin_t *this)
{


	return 0;
}

/*--------------------------------------------------------- */
