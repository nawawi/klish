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
#include <dlfcn.h>

/**********************************************************
 * SYM functions                                          *
 **********************************************************/

/*--------------------------------------------------------- */
int clish_sym_compare(const void *first, const void *second)
{
	const clish_sym_t *f = (const clish_sym_t *)first;
	const clish_sym_t *s = (const clish_sym_t *)second;

	return strcmp(f->name, s->name);
}

/*--------------------------------------------------------- */
clish_sym_t *clish_sym_new(const char *name, clish_plugin_fn_t *func)
{
	clish_sym_t *this;

	this = malloc(sizeof(*this));
	this->name = lub_string_dup(name);
	this->func = func;

	return this;
}

/*--------------------------------------------------------- */
void clish_sym_free(clish_sym_t *this)
{
	if (!this)
		return;
	lub_string_free(this->name);
	free(this);
}

/*--------------------------------------------------------- */
void clish_sym__set_func(clish_sym_t *this, clish_plugin_fn_t *func)
{
	this->func = func;
}

/*--------------------------------------------------------- */
clish_plugin_fn_t *clish_sym__get_func(clish_sym_t *this)
{
	return this->func;
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
	this->dlhan = NULL;
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
	if (this->dlhan)
		dlclose(this->dlhan);

	free(this);
}

/*--------------------------------------------------------- */
int clish_plugin_add_sym(clish_plugin_t *this,
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
clish_plugin_fn_t *clish_plugin_get_sym(clish_plugin_t *this, const char *name)
{
	lub_list_node_t *iter;
	clish_sym_t *sym;

	/* Iterate elements */
	for(iter = lub_list__get_head(this->syms);
		iter; iter = lub_list_node__get_next(iter)) {
		int res;
		sym = (clish_sym_t *)lub_list_node__get_data(iter);
		res = strcmp(sym->name, name);
		if (!res)
			return clish_sym__get_func(sym);
		if (res > 0) /* No chances to find name */
			break;
	}

	return NULL;
}

/*--------------------------------------------------------- */
void *clish_plugin_load(clish_plugin_t *this)
{
	clish_plugin_init_t *plugin_init;

	if (!this)
		return NULL;

	if (!(this->dlhan = dlopen(this->file, RTLD_NOW | RTLD_GLOBAL)))
		return NULL;
	plugin_init = (clish_plugin_init_t *)dlsym(this->dlhan, CLISH_PLUGIN_INIT);
	if (!plugin_init) {
		dlclose(this->dlhan);
		this->dlhan = NULL;
		return NULL;
	}
	plugin_init(this);

	return this->dlhan;
}

/*--------------------------------------------------------- */
char *clish_plugin__get_name(const clish_plugin_t *this)
{
	return this->name;
}

/*--------------------------------------------------------- */
char *clish_plugin__get_file(const clish_plugin_t *this)
{
	return this->file;
}

/*--------------------------------------------------------- */
