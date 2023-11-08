#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <faux/list.h>
#include <faux/error.h>
#include <klish/khelper.h>
#include <klish/kplugin.h>
#include <klish/kentry.h>
#include <klish/kscheme.h>
#include <klish/kcontext.h>
#include <klish/kustore.h>


struct kscheme_s {
	faux_list_t *plugins;
	faux_list_t *entrys;
	kustore_t *ustore;
};

// Simple methods

// PLUGIN list
KGET(scheme, faux_list_t *, plugins);
KCMP_NESTED(scheme, plugin, name);
KCMP_NESTED_BY_KEY(scheme, plugin, name);
KADD_NESTED(scheme, kplugin_t *, plugins);
KFIND_NESTED(scheme, plugin);
KNESTED_LEN(scheme, plugins);
KNESTED_ITER(scheme, plugins);
KNESTED_EACH(scheme, kplugin_t *, plugins);

// ENTRY list
KGET(scheme, faux_list_t *, entrys);
KCMP_NESTED(scheme, entry, name);
KCMP_NESTED_BY_KEY(scheme, entry, name);
KADD_NESTED(scheme, kentry_t *, entrys);
KFIND_NESTED(scheme, entry);
KNESTED_LEN(scheme, entrys);
KNESTED_ITER(scheme, entrys);
KNESTED_EACH(scheme, kentry_t *, entrys);


kscheme_t *kscheme_new(void)
{
	kscheme_t *scheme = NULL;

	scheme = faux_zmalloc(sizeof(*scheme));
	assert(scheme);
	if (!scheme)
		return NULL;

	// PLUGIN list
	scheme->plugins = faux_list_new(FAUX_LIST_SORTED, FAUX_LIST_UNIQUE,
		kscheme_plugin_compare, kscheme_plugin_kcompare,
		(void (*)(void *))kplugin_free);
	assert(scheme->plugins);

	// ENTRY list
	// Must be unsorted because order is important
	scheme->entrys = faux_list_new(FAUX_LIST_UNSORTED, FAUX_LIST_UNIQUE,
		kscheme_entry_compare, kscheme_entry_kcompare,
		(void (*)(void *))kentry_free);
	assert(scheme->entrys);

	// User store
	scheme->ustore = kustore_new();
	assert(scheme->ustore);

	return scheme;
}


void kscheme_free(kscheme_t *scheme)
{
	if (!scheme)
		return;

	faux_list_free(scheme->plugins);
	faux_list_free(scheme->entrys);
	kustore_free(scheme->ustore);

	faux_free(scheme);
}

#define TAG "PLUGIN"

static bool_t kscheme_load_plugins(kscheme_t *scheme, kcontext_t *context,
	faux_error_t *error)
{
	bool_t retcode = BOOL_TRUE;
	kscheme_plugins_node_t *iter = NULL;
	kplugin_t *plugin = NULL;

	assert(scheme);
	if (!scheme)
		return BOOL_FALSE;
	assert(scheme->plugins);
	if (!context)
		return BOOL_FALSE;

	iter = kscheme_plugins_iter(scheme);
	while ((plugin = kscheme_plugins_each(&iter))) {
		int init_retcode = 0;
		if (!kplugin_load(plugin)) {
			faux_error_sprintf(error,
				TAG ": Can't load plugin \"%s\"",
				kplugin_name(plugin));
			retcode = BOOL_FALSE;
			continue; // Try to load all plugins
		}
		kcontext_set_type(context, KCONTEXT_TYPE_PLUGIN_INIT);
		kcontext_set_plugin(context, plugin);
		if ((init_retcode = kplugin_init(plugin, context)) < 0) {
			faux_error_sprintf(error,
				TAG ": Can't init plugin \"%s\" (%d)",
				kplugin_name(plugin), init_retcode);
			retcode = BOOL_FALSE;
			continue;
		}
	}

	return retcode;
}


static bool_t kscheme_fini_plugins(kscheme_t *scheme, kcontext_t *context,
	faux_error_t *error)
{
	kscheme_plugins_node_t *iter = NULL;
	kplugin_t *plugin = NULL;

	assert(scheme);
	if (!scheme)
		return BOOL_FALSE;
	assert(scheme->plugins);
	if (!context)
		return BOOL_FALSE;

	iter = kscheme_plugins_iter(scheme);
	while ((plugin = kscheme_plugins_each(&iter))) {
		int fini_retcode = -1;
		kcontext_set_type(context, KCONTEXT_TYPE_PLUGIN_FINI);
		kcontext_set_plugin(context, plugin);
		if ((fini_retcode = kplugin_fini(plugin, context)) < 0) {
			faux_error_sprintf(error,
				TAG ": Can't fini plugin \"%s\" (%d)",
				kplugin_name(plugin), fini_retcode);
		}
	}

	return BOOL_TRUE;
}


bool_t kscheme_fini(kscheme_t *scheme, kcontext_t *context, faux_error_t *error)
{

	assert(scheme);
	if (!scheme)
		return BOOL_FALSE;
	if (!context)
		return BOOL_FALSE;

	if (!kscheme_fini_plugins(scheme, context, error))
		return BOOL_FALSE;

	return BOOL_TRUE;
}


static ksym_t *kscheme_find_sym(const kscheme_t *scheme, const char *name,
	kplugin_t **src_plugin)
{
	char *saveptr = NULL;
	const char *delim = "@";
	char *plugin_name = NULL;
	char *cmd_name = NULL;
	char *full_name = NULL;
	ksym_t *sym = NULL;
	kplugin_t *plugin = NULL; // Source of sym

	assert(scheme);
	if (!scheme)
		return NULL;
	assert(name);
	if (!name)
		return NULL;

	// Parse full name to get sym name and optional plugin name
	full_name = faux_str_dup(name);
	cmd_name = strtok_r(full_name, delim, &saveptr);
	if (!cmd_name) {
		faux_str_free(full_name);
		return NULL;
	}
	plugin_name = strtok_r(NULL, delim, &saveptr);

	// Search for symbol within specified PLUGIN only
	if (plugin_name) {
		plugin = kscheme_find_plugin(scheme, plugin_name);
		if (!plugin) {
			faux_str_free(full_name);
			return NULL;
		}
		sym = kplugin_find_sym(plugin, cmd_name);

	// Search for symbol within all PLUGINs
	} else {
		kscheme_plugins_node_t *iter = NULL;
		iter = kscheme_plugins_iter(scheme);
		while ((plugin = kscheme_plugins_each(&iter))) {
			sym = kplugin_find_sym(plugin, cmd_name);
			if (sym)
				break;
		}
	}

	if (sym && src_plugin)
		*src_plugin = plugin;

	faux_str_free(full_name);

	return sym;
}


bool_t kscheme_prepare_action_list(kscheme_t *scheme, kentry_t *entry,
	faux_error_t *error)
{
	faux_list_node_t *iter = NULL;
	kaction_t *action = NULL;
	bool_t retcode = BOOL_TRUE;
	faux_list_t *action_list = NULL;

	assert(scheme);
	if (!scheme)
		return BOOL_FALSE;
	assert(entry);
	if (!entry)
		return BOOL_FALSE;
	action_list = kentry_actions(entry);
	assert(action_list);
	if (!action_list)
		return BOOL_FALSE;
	if (faux_list_is_empty(action_list))
		return BOOL_TRUE;

	iter = faux_list_head(action_list);
	while ((action = (kaction_t *)faux_list_each(&iter))) {
		ksym_t *sym = NULL;
		kplugin_t *plugin = NULL;
		const char *sym_ref = kaction_sym_ref(action);

		sym = kscheme_find_sym(scheme, sym_ref, &plugin);
		if (!sym) {
			faux_error_sprintf(error, "Can't find symbol \"%s\"",
				sym_ref);
			retcode = BOOL_FALSE;
			continue;
		}
		kaction_set_sym(action, sym);
		kaction_set_plugin(action, plugin);
		// Filter can't contain sync symbols
		if ((kentry_filter(entry) != KENTRY_FILTER_FALSE) &&
			kaction_is_sync(action)) {
			faux_error_sprintf(error, "Filter \"%s\" can't contain "
				"sync symbol \"%s\"",
				kentry_name(entry), sym_ref);
			retcode = BOOL_FALSE;
			continue;
		}
	}

	return retcode;
}


kentry_t *kscheme_find_entry_by_path(const kscheme_t *scheme, const char *name)
{
	char *saveptr = NULL;
	const char *delim = "/";
	char *entry_name = NULL;
	char *full_name = NULL;
	kentry_t *entry = NULL;

	assert(scheme);
	if (!scheme)
		return NULL;
	assert(name);
	if (!name)
		return NULL;

	// Get first component of ENTRY path. It will be searched for
	// within scheme.
	full_name = faux_str_dup(name);
	entry_name = strtok_r(full_name, delim, &saveptr);
	if (!entry_name) {
		faux_str_free(full_name);
		return NULL;
	}
	entry = kscheme_find_entry(scheme, entry_name);
	if (!entry) {
		faux_str_free(full_name);
		return NULL;
	}

	// Search nested ENTRYs
	while ((entry_name = strtok_r(NULL, delim, &saveptr))) {
		entry = kentry_find_entry(entry, entry_name);
		if (!entry)
			break;
	}

	faux_str_free(full_name);

	return entry;
}


bool_t kscheme_prepare_entry(kscheme_t *scheme, kentry_t *entry,
	faux_error_t *error) {
	kentry_entrys_node_t *iter = NULL;
	kentry_t *nested_entry = NULL;
	bool_t retcode = BOOL_TRUE;
	const char *ref = NULL;
	kentry_t *ref_entry = NULL;

	assert(scheme);
	if (!scheme)
		return BOOL_FALSE;
	assert(entry);
	if (!entry)
		return BOOL_FALSE;

	// Firstly if ENTRY is link to another ENTRY then make a copy
	if (kentry_ref_str(entry)) {
		ref_entry = entry;
		// Find the most deep real (non-link) object to reference to it
		// i.e. the link can't reference a link.
		while ((ref = kentry_ref_str(ref_entry))) {
			ref_entry = kscheme_find_entry_by_path(scheme, ref);
			if (!ref_entry) {
				faux_error_sprintf(error, "Can't find ENTRY \"%s\"", ref);
				return BOOL_FALSE;
			}
		}
		if (!kentry_link(entry, ref_entry)) {
			faux_error_sprintf(error, "Can't create link to ENTRY \"%s\"", ref);
			return BOOL_FALSE;
		}
		return BOOL_TRUE;
	}

	// ACTIONs
	if (!kscheme_prepare_action_list(scheme, entry, error))
		retcode = BOOL_FALSE;

	// Create fast links to nested entries with special purposes. Do it
	// before preparing child elements because some fields can be inhereted
	// by child from its parent
	iter = kentry_entrys_iter(entry);
	while ((nested_entry = kentry_entrys_each(&iter)))
		kentry_set_nested_by_purpose(entry,
			kentry_purpose(nested_entry), nested_entry);
	// Inherit LOG from parent if it's not specified explicitly
	if (!kentry_nested_by_purpose(entry, KENTRY_PURPOSE_LOG)) {
		kentry_t *parent = kentry_parent(entry);
		if (parent) { // Get LOG from parent entry
			kentry_set_nested_by_purpose(entry, KENTRY_PURPOSE_LOG,
				kentry_nested_by_purpose(parent,
				KENTRY_PURPOSE_LOG));
		} else { // High level entries has no parents
			iter = kscheme_entrys_iter(scheme);
			while ((nested_entry = kscheme_entrys_each(&iter))) {
				if (kentry_purpose(nested_entry) !=
					KENTRY_PURPOSE_LOG)
					continue;
				kentry_set_nested_by_purpose(entry,
					KENTRY_PURPOSE_LOG, nested_entry);
			}
		}
	}

	// Process nested ENTRYs
	iter = kentry_entrys_iter(entry);
	while ((nested_entry = kentry_entrys_each(&iter)))
		if (!kscheme_prepare_entry(scheme, nested_entry, error))
			retcode = BOOL_FALSE;

	return retcode;
}


/** @brief Prepares schema for execution.
 *
 * It loads plugins, link unresolved symbols, then iterates all the
 * objects and link them to each other, check access
 * permissions. Without this function the schema is not fully functional.
 */
bool_t kscheme_prepare(kscheme_t *scheme, kcontext_t *context, faux_error_t *error)
{
	kscheme_entrys_node_t *entrys_iter = NULL;
	kentry_t *entry = NULL;

	assert(scheme);
	if (!scheme)
		return BOOL_FALSE;
	if (!context)
		return BOOL_FALSE;

	if (!kscheme_load_plugins(scheme, context, error))
		return BOOL_FALSE;

	// Iterate ENTRYs
	entrys_iter = kscheme_entrys_iter(scheme);
	while ((entry = kscheme_entrys_each(&entrys_iter))) {
		if (!kscheme_prepare_entry(scheme, entry, error))
			return BOOL_FALSE;
	}

	return BOOL_TRUE;
}


bool_t kscheme_named_udata_new(kscheme_t *scheme,
	const char *name, void *data, kudata_data_free_fn free_fn)
{
	kudata_t *udata = NULL;

	assert(scheme);
	if (!scheme)
		return BOOL_FALSE;
	assert(scheme->ustore);

	udata = kustore_slot_new(scheme->ustore, name, data, free_fn);
	if (!udata)
		return BOOL_FALSE;

	return BOOL_TRUE;
}


void *kscheme_named_udata(kscheme_t *scheme, const char *name)
{
	assert(scheme);
	if (!scheme)
		return BOOL_FALSE;
	assert(scheme->ustore);

	return kustore_slot_data(scheme->ustore, name);
}


bool_t kscheme_init_session_plugins(kscheme_t *scheme, kcontext_t *context,
	faux_error_t *error)
{
	bool_t retcode = BOOL_TRUE;
	kscheme_plugins_node_t *iter = NULL;
	kplugin_t *plugin = NULL;

	assert(scheme);
	if (!scheme)
		return BOOL_FALSE;
	assert(scheme->plugins);
	if (!context)
		return BOOL_FALSE;

	iter = kscheme_plugins_iter(scheme);
	while ((plugin = kscheme_plugins_each(&iter))) {
		int init_retcode = 0;
		kcontext_set_type(context, KCONTEXT_TYPE_PLUGIN_INIT);
		kcontext_set_plugin(context, plugin);
		if ((init_retcode = kplugin_init_session(plugin, context)) < 0) {
			faux_error_sprintf(error,
				TAG ": Can't init session for plugin \"%s\" (%d)",
				kplugin_name(plugin), init_retcode);
			retcode = BOOL_FALSE;
			continue;
		}
	}

	return retcode;
}


bool_t kscheme_fini_session_plugins(kscheme_t *scheme, kcontext_t *context,
	faux_error_t *error)
{
	kscheme_plugins_node_t *iter = NULL;
	kplugin_t *plugin = NULL;

	assert(scheme);
	if (!scheme)
		return BOOL_FALSE;
	assert(scheme->plugins);
	if (!context)
		return BOOL_FALSE;

	iter = kscheme_plugins_iter(scheme);
	while ((plugin = kscheme_plugins_each(&iter))) {
		int fini_retcode = -1;
		kcontext_set_type(context, KCONTEXT_TYPE_PLUGIN_FINI);
		kcontext_set_plugin(context, plugin);
		if ((fini_retcode = kplugin_fini_session(plugin, context)) < 0) {
			faux_error_sprintf(error,
				TAG ": Can't fini session for plugin \"%s\" (%d)",
				kplugin_name(plugin), fini_retcode);
		}
	}

	return BOOL_TRUE;
}
