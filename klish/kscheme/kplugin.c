#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <dlfcn.h>

#include <faux/str.h>
#include <faux/list.h>
#include <faux/conv.h>
#include <faux/error.h>
#include <klish/khelper.h>
#include <klish/kplugin.h>
#include <klish/ksym.h>
#include <klish/kcontext_base.h>


struct kplugin_s {
	char *name;
	char *id;
	char *file;
	char *conf;
	uint8_t major;
	uint8_t minor;
	void *dlhan; // dlopen() handler
	ksym_fn init_fn;
	ksym_fn fini_fn;
	void *udata; // User data
	faux_list_t *syms;
};


// Simple methods

// Name
KGET_STR(plugin, name);

// ID
KGET_STR(plugin, id);
KSET_STR(plugin, id);

// File
KGET_STR(plugin, file);
KSET_STR(plugin, file);

// Conf
KGET_STR(plugin, conf);
KSET_STR(plugin, conf);

// Version major number
KGET(plugin, uint8_t, major);
KSET(plugin, uint8_t, major);

// Version minor number
KGET(plugin, uint8_t, minor);
KSET(plugin, uint8_t, minor);

// User data
KGET(plugin, void *, udata);
KSET(plugin, void *, udata);

// COMMAND list
static KCMP_NESTED(plugin, sym, name);
static KCMP_NESTED_BY_KEY(plugin, sym, name);
KADD_NESTED(plugin, sym);
KFIND_NESTED(plugin, sym);
KNESTED_LEN(plugin, sym);
KNESTED_ITER(plugin, sym);
KNESTED_EACH(plugin, sym);


kplugin_t *kplugin_new(const char *name)
{
	kplugin_t *plugin = NULL;

	if (faux_str_is_empty(name))
		return NULL;

	plugin = faux_zmalloc(sizeof(*plugin));
	assert(plugin);
	if (!plugin)
		return NULL;

	// Initialize
	plugin->name = faux_str_dup(name);
	plugin->id = NULL;
	plugin->file = NULL;
	plugin->conf = NULL;
	plugin->major = 0;
	plugin->minor = 0;
	plugin->dlhan = NULL;
	plugin->init_fn = NULL;
	plugin->fini_fn = NULL;
	plugin->udata = NULL;

	// SYM list
	plugin->syms = faux_list_new(FAUX_LIST_SORTED, FAUX_LIST_UNIQUE,
		kplugin_sym_compare, kplugin_sym_kcompare,
		(void (*)(void *))ksym_free);
	assert(plugin->syms);

	return plugin;
}


void kplugin_free(kplugin_t *plugin)
{
	if (!plugin)
		return;

	faux_str_free(plugin->name);
	faux_str_free(plugin->id);
	faux_str_free(plugin->file);
	faux_str_free(plugin->conf);
	faux_list_free(plugin->syms);

	if (plugin->dlhan)
		dlclose(plugin->dlhan);

	faux_free(plugin);
}


bool_t kplugin_load(kplugin_t *plugin)
{
	char *file_name = NULL;
	char *init_name = NULL;
	char *fini_name = NULL;
	char *major_name = NULL;
	char *minor_name = NULL;
	int flag = RTLD_NOW | RTLD_LOCAL;
	const char *id = NULL;
	bool_t retcode = BOOL_FALSE;
	uint8_t *ver = NULL;

	assert(plugin);
	if (!plugin)
		return BOOL_FALSE;

	if (kplugin_id(plugin))
		id = kplugin_id(plugin);
	else
		id = kplugin_name(plugin);

	// Shared object file name
	if (kplugin_file(plugin))
		file_name = faux_str_dup(kplugin_file(plugin));
	else
		file_name = faux_str_sprintf(KPLUGIN_SONAME_FMT, id);

	// Symbol names
	major_name = faux_str_sprintf(KPLUGIN_MAJOR_FMT, id);
	minor_name = faux_str_sprintf(KPLUGIN_MINOR_FMT, id);
	init_name = faux_str_sprintf(KPLUGIN_INIT_FMT, id);
	fini_name = faux_str_sprintf(KPLUGIN_FINI_FMT, id);

	// Open shared object
	plugin->dlhan = dlopen(file_name, flag);
	if (!plugin->dlhan) {
//		fprintf(stderr, "Error: Can't open plugin \"%s\": %s\n",
//			this->name, dlerror());
		goto err;
	}

	// Get plugin version
	ver = (uint8_t *)dlsym(plugin->dlhan, major_name);
	if (!ver)
		goto err;
	kplugin_set_major(plugin, *ver);
	ver = (uint8_t *)dlsym(plugin->dlhan, minor_name);
	if (!ver)
		goto err;
	kplugin_set_minor(plugin, *ver);

	// Get plugin init function
	plugin->init_fn = dlsym(plugin->dlhan, init_name);
	if (!plugin->init_fn) {
//		fprintf(stderr, "Error: Can't get plugin \"%s\" init function: %s\n",
//			this->name, dlerror());
		goto err;
	}

	// Get plugin fini function
	plugin->fini_fn = dlsym(plugin->dlhan, fini_name);

	retcode = BOOL_TRUE;
err:
	faux_str_free(file_name);
	faux_str_free(major_name);
	faux_str_free(minor_name);
	faux_str_free(init_name);
	faux_str_free(fini_name);

	if (!retcode && plugin->dlhan)
		dlclose(plugin->dlhan);

	return retcode;
}


int kplugin_init(kplugin_t *plugin, kcontext_t *context)
{
	assert(plugin);
	if (!plugin)
		return -1;
	assert(context);
	if (!context)
		return -1;

	if (!plugin->init_fn)
		return -1;
	// Be sure the context type is appropriate one
	kcontext_set_type(context, KCONTEXT_PLUGIN_INIT);

	return plugin->init_fn(context);
}


int kplugin_fini(kplugin_t *plugin, kcontext_t *context)
{
	assert(plugin);
	if (!plugin)
		return -1;
	assert(context);
	if (!context)
		return -1;

	if (!plugin->fini_fn)
		return 0; // Fini function is not mandatory so it's ok
	// Be sure the context type is appropriate one
	kcontext_set_type(context, KCONTEXT_PLUGIN_FINI);

	return plugin->fini_fn(context);
}
