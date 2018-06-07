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
clish_plugin_t *clish_plugin_new(const char *name, void *userdata)
{
	clish_plugin_t *this;

	this = malloc(sizeof(*this));

	this->name = lub_string_dup(name);
	this->conf = NULL;
	this->alias = NULL;
	this->file = NULL;
	this->builtin_flag = BOOL_FALSE; /* The plugin is shared object by default */
	this->dlhan = NULL;
	/* Initialise the list of symbols */
	this->syms = lub_list_new(clish_sym_compare, clish_sym_free);
	/* Constructor and destructor */
	this->init = NULL;
	this->fini = NULL;
	/* Flags */
	this->rtld_global = BOOL_FALSE; /* The dlopen() use RTLD_LOCAL by default */
	/* Userdata */
	this->userdata = userdata;

	return this;
}

/*--------------------------------------------------------- */
void clish_plugin_free(void *plugin)
{
	clish_plugin_t *this = (clish_plugin_t *)plugin;

	if (!this)
		return;

	/* Execute destructor */
	if (this->fini)
		this->fini(this->userdata, this);

	lub_string_free(this->name);
	lub_string_free(this->alias);
	lub_string_free(this->file);
	lub_string_free(this->conf);

	/* Free symbol list */
	lub_list_free_all(this->syms);
#ifdef HAVE_DLFCN_H
	if (this->dlhan)
		dlclose(this->dlhan);
#endif
	free(this);
}

/*--------------------------------------------------------- */
clish_sym_t *clish_plugin_add_generic(clish_plugin_t *this,
	void *func, const char *name, int type, bool_t permanent)
{
	clish_sym_t *sym;

	if (!name || !func)
		return NULL;

	if (!(sym = clish_sym_new(name, func, type)))
		return NULL;
	clish_sym__set_plugin(sym, this);
	clish_sym__set_permanent(sym, permanent);
	lub_list_add(this->syms, sym);

	return sym;
}

/*--------------------------------------------------------- */
clish_sym_t *clish_plugin_add_sym(clish_plugin_t *this,
	clish_hook_action_fn_t *func, const char *name)
{
	return clish_plugin_add_generic(this, func,
		name, CLISH_SYM_TYPE_ACTION, BOOL_FALSE);
}

/*--------------------------------------------------------- */
/* Add permanent symbol (can't be turned off by dry-run) */
clish_sym_t *clish_plugin_add_psym(clish_plugin_t *this,
	clish_hook_action_fn_t *func, const char *name)
{
	return clish_plugin_add_generic(this, func,
		name, CLISH_SYM_TYPE_ACTION, BOOL_TRUE);
}

/*--------------------------------------------------------- */
clish_sym_t *clish_plugin_add_osym(clish_plugin_t *this,
	clish_hook_oaction_fn_t *func, const char *name)
{
	clish_sym_t *s;

	if (!(s = clish_plugin_add_generic(this, func,
		name, CLISH_SYM_TYPE_ACTION, BOOL_FALSE)))
		return s;
	clish_sym__set_api(s, CLISH_SYM_API_STDOUT);

	return s;
}

/*--------------------------------------------------------- */
/* Add permanent symbol (can't be turned off by dry-run) */
clish_sym_t *clish_plugin_add_posym(clish_plugin_t *this,
	clish_hook_oaction_fn_t *func, const char *name)
{
	clish_sym_t *s;

	if (!(s = clish_plugin_add_generic(this, func,
		name, CLISH_SYM_TYPE_ACTION, BOOL_TRUE)))
		return s;
	clish_sym__set_api(s, CLISH_SYM_API_STDOUT);

	return s;
}

/*--------------------------------------------------------- */
clish_sym_t *clish_plugin_add_hook(clish_plugin_t *this,
	void *func, const char *name, int type)
{
	return clish_plugin_add_generic(this, func,
		name, type, BOOL_FALSE);
}

/*--------------------------------------------------------- */
clish_sym_t *clish_plugin_add_phook(clish_plugin_t *this,
	void *func, const char *name, int type)
{
	return clish_plugin_add_generic(this, func,
		name, type, BOOL_TRUE);
}

/*--------------------------------------------------------- */
clish_sym_t *clish_plugin_get_sym(clish_plugin_t *this, const char *name, int type)
{
	lub_list_node_t *iter;
	clish_sym_t *sym;

	/* Iterate elements */
	for(iter = lub_list__get_head(this->syms);
		iter; iter = lub_list_node__get_next(iter)) {
		int res;
		sym = (clish_sym_t *)lub_list_node__get_data(iter);
		res = strcmp(clish_sym__get_name(sym), name);
		if (!res && ((CLISH_SYM_TYPE_NONE == type) || (clish_sym__get_type(sym) == type)))
			return sym;
		if (res > 0) /* No chances to find name */
			break;
	}

	return NULL;
}

/*--------------------------------------------------------- */
static int clish_plugin_load_shared(clish_plugin_t *this)
{
#ifdef HAVE_DLFCN_H
	char *file = NULL; /* Plugin so file name */
	char *init_name = NULL; /* Init function name */
	int flag = RTLD_NOW;

	if (this->file) {
		file = lub_string_dup(this->file);
	} else {
		lub_string_cat(&file, "clish_plugin_");
		lub_string_cat(&file, this->name);
		lub_string_cat(&file, ".so");
	}

	/* Open dynamic library */
	if (clish_plugin__get_rtld_global(this))
		flag |= RTLD_GLOBAL;
	else
		flag |= RTLD_LOCAL;
	this->dlhan = dlopen(file, flag);
	lub_string_free(file);
	if (!this->dlhan) {
		fprintf(stderr, "Error: Can't open plugin \"%s\": %s\n",
			this->name, dlerror());
		return -1;
	}

	/* Get plugin init function */
	lub_string_cat(&init_name, CLISH_PLUGIN_INIT_NAME_PREFIX);
	lub_string_cat(&init_name, this->name);
	lub_string_cat(&init_name, CLISH_PLUGIN_INIT_NAME_SUFFIX);
	this->init = (clish_plugin_init_t *)dlsym(this->dlhan, init_name);
	lub_string_free(init_name);
	if (!this->init) {
		fprintf(stderr, "Error: Can't get plugin \"%s\" init function: %s\n",
			this->name, dlerror());
		return -1;
	}

	return 0;
#else /* HAVE_DLFCN_H */
	/* We have no any dl functions. */
	fprintf(stderr, "Error: Can't get plugin \"%s\" init function.\n",
		this->name);
	return -1;
#endif /* HAVE_DLFCN_H */
}

/*--------------------------------------------------------- */
int clish_plugin_load(clish_plugin_t *this)
{
	int res;

	if (!this)
		return -1;
	assert(this->name);

	/* Builtin plugins already have init function. */
	if (!this->builtin_flag) {
		if (clish_plugin_load_shared(this) < 0)
			return -1;
	}
	if (!this->init) {
		fprintf(stderr, "Error: PLUGIN %s has no init function\n",
			this->name);
		return -1;
	}
	/* Execute init function */
	if ((res = this->init(this->userdata, this)))
		fprintf(stderr, "Error: Plugin %s init retcode: %d\n",
			this->name, res);

	return res;
}

CLISH_SET(plugin, clish_plugin_fini_t *, fini);
CLISH_GET(plugin, clish_plugin_fini_t *, fini);
CLISH_SET(plugin, clish_plugin_init_t *, init);
CLISH_GET(plugin, clish_plugin_init_t *, init);
CLISH_GET_STR(plugin, name);
CLISH_SET_STR(plugin, alias);
CLISH_GET_STR(plugin, alias);
CLISH_SET_STR(plugin, file);
CLISH_GET_STR(plugin, file);
CLISH_SET_STR(plugin, conf);
CLISH_GET_STR(plugin, conf);
CLISH_SET(plugin, bool_t, builtin_flag);
CLISH_GET(plugin, bool_t, builtin_flag);
CLISH_SET(plugin, bool_t, rtld_global);
CLISH_GET(plugin, bool_t, rtld_global);

/*--------------------------------------------------------- */
_CLISH_GET_STR(plugin, pubname)
{
	assert(inst);
	return (inst->alias ? inst->alias : inst->name);
}
