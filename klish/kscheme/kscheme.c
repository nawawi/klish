#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <faux/list.h>
#include <faux/error.h>
#include <klish/khelper.h>
#include <klish/kplugin.h>
#include <klish/kptype.h>
#include <klish/kview.h>
#include <klish/knspace.h>
#include <klish/kentry.h>
#include <klish/kscheme.h>
#include <klish/kcontext.h>


struct kscheme_s {
	faux_list_t *plugins;
	faux_list_t *ptypes;
	faux_list_t *views;
	faux_list_t *entrys;
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

// PTYPE list
KGET(scheme, faux_list_t *, ptypes);
KCMP_NESTED(scheme, ptype, name);
KCMP_NESTED_BY_KEY(scheme, ptype, name);
KADD_NESTED(scheme, kptype_t *, ptypes);
KFIND_NESTED(scheme, ptype);
KNESTED_LEN(scheme, ptypes);
KNESTED_ITER(scheme, ptypes);
KNESTED_EACH(scheme, kptype_t *, ptypes);

// VIEW list
KGET(scheme, faux_list_t *, views);
KCMP_NESTED(scheme, view, name);
KCMP_NESTED_BY_KEY(scheme, view, name);
KADD_NESTED(scheme, kview_t *, views);
KFIND_NESTED(scheme, view);
KNESTED_LEN(scheme, views);
KNESTED_ITER(scheme, views);
KNESTED_EACH(scheme, kview_t *, views);

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

	// PTYPE list
	scheme->ptypes = faux_list_new(FAUX_LIST_SORTED, FAUX_LIST_UNIQUE,
		kscheme_ptype_compare, kscheme_ptype_kcompare,
		(void (*)(void *))kptype_free);
	assert(scheme->ptypes);

	// VIEW list
	scheme->views = faux_list_new(FAUX_LIST_SORTED, FAUX_LIST_UNIQUE,
		kscheme_view_compare, kscheme_view_kcompare,
		(void (*)(void *))kview_free);
	assert(scheme->views);

	// ENTRY list
	// Must be unsorted because order is important
	scheme->entrys = faux_list_new(FAUX_LIST_UNSORTED, FAUX_LIST_UNIQUE,
		kscheme_entry_compare, kscheme_entry_kcompare,
		(void (*)(void *))kentry_free);
	assert(scheme->entrys);


	return scheme;
}


void kscheme_free(kscheme_t *scheme)
{
	if (!scheme)
		return;

	faux_list_free(scheme->plugins);
	faux_list_free(scheme->ptypes);
	faux_list_free(scheme->views);
	faux_list_free(scheme->entrys);

	faux_free(scheme);
}

#define TAG "PLUGIN"

bool_t kscheme_load_plugins(kscheme_t *scheme, kcontext_t *context,
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
		kcontext_set_type(context, KCONTEXT_PLUGIN_INIT);
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


bool_t kscheme_fini_plugins(kscheme_t *scheme, kcontext_t *context,
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
		kcontext_set_type(context, KCONTEXT_PLUGIN_FINI);
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


ksym_t *kscheme_find_sym(const kscheme_t *scheme, const char *name)
{
	char *saveptr = NULL;
	const char *delim = "@";
	char *plugin_name = NULL;
	char *cmd_name = NULL;
	char *full_name = NULL;
	ksym_t *sym = NULL;

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
		kplugin_t *plugin = NULL;
		plugin = kscheme_find_plugin(scheme, plugin_name);
		if (!plugin) {
			faux_str_free(full_name);
			return NULL;
		}
		sym = kplugin_find_sym(plugin, cmd_name);

	// Search for symbol within all PLUGINs
	} else {
		kscheme_plugins_node_t *iter = NULL;
		kplugin_t *plugin = NULL;
		iter = kscheme_plugins_iter(scheme);
		while ((plugin = kscheme_plugins_each(&iter))) {
			sym = kplugin_find_sym(plugin, cmd_name);
			if (sym)
				break;
		}
	}

	faux_str_free(full_name);

	return sym;
}


bool_t kscheme_prepare_action_list(kscheme_t *scheme, faux_list_t *action_list,
	faux_error_t *error) {
	faux_list_node_t *iter = NULL;
	kaction_t *action = NULL;
	bool_t retcode = BOOL_TRUE;

	assert(scheme);
	if (!scheme)
		return BOOL_FALSE;
	assert(action_list);
	if (!action_list)
		return BOOL_FALSE;
	if (faux_list_is_empty(action_list))
		return BOOL_TRUE;

	iter = faux_list_head(action_list);
	while ((action = (kaction_t *)faux_list_each(&iter))) {
		ksym_t *sym = NULL;
		const char *sym_ref = kaction_sym_ref(action);
		sym = kscheme_find_sym(scheme, sym_ref);
		if (!sym) {
			faux_error_sprintf(error, "Can't find symbol \"%s\"",
				sym_ref);
			retcode = BOOL_FALSE;
			continue;
		}
		kaction_set_sym(action, sym);
	}

	return retcode;
}


bool_t kscheme_prepare_param_list(kscheme_t *scheme, faux_list_t *param_list,
	faux_error_t *error) {
	faux_list_node_t *iter = NULL;
	kparam_t *param = NULL;
	bool_t retcode = BOOL_TRUE;

	assert(scheme);
	if (!scheme)
		return BOOL_FALSE;
	assert(param_list);
	if (!param_list)
		return BOOL_FALSE;
	if (faux_list_is_empty(param_list))
		return BOOL_TRUE;

	iter = faux_list_head(param_list);
	while ((param = (kparam_t *)faux_list_each(&iter))) {
		kptype_t *ptype = NULL;
		const char *ptype_ref = kparam_ptype_ref(param);
		ptype = kscheme_find_ptype(scheme, ptype_ref);
		if (!ptype) {
			faux_error_sprintf(error, "Can't find ptype \"%s\"",
				ptype_ref);
			retcode = BOOL_FALSE;
			continue;
		}
		kparam_set_ptype(param, ptype);
		// Nested PARAMs
		if (!kscheme_prepare_param_list(scheme, kparam_params(param),
			error))
			retcode = BOOL_FALSE;
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
	const char *ptype_str = NULL;

	assert(scheme);
	if (!scheme)
		return BOOL_FALSE;
	assert(entry);
	if (!entry)
		return BOOL_FALSE;

	// Firstly if ENTRY is link to another ENTRY then make a copy
	if (kentry_ref_str(entry)) {
		ref_entry = entry;
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
	}

	// Resolve ptype's ENTRY
	if ((ptype_str = kentry_ptype_str(entry))) {
		ref_entry = kscheme_find_entry_by_path(scheme, ptype_str);
		if (!ref_entry) {
			faux_error_sprintf(error, "Can't find ENTRY \"%s\" for ptype",
				ptype_str);
			retcode = BOOL_FALSE;
		}
		kentry_set_ptype(entry, ref_entry);
	}

	// ACTIONs
	if (!kscheme_prepare_action_list(scheme, kentry_actions(entry), error))
		retcode = BOOL_FALSE;

	// Process nested ENTRYs
	iter = kentry_entrys_iter(entry);
	while ((nested_entry = kentry_entrys_each(&iter))) {
		if (!kscheme_prepare_entry(scheme, nested_entry, error))
			retcode = BOOL_FALSE;
	}

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
	kscheme_views_node_t *views_iter = NULL;
	kview_t *view = NULL;
	kscheme_entrys_node_t *entrys_iter = NULL;
	kentry_t *entry = NULL;

	assert(scheme);
	if (!scheme)
		return BOOL_FALSE;
	if (!context)
		return BOOL_FALSE;

	if (!kscheme_load_plugins(scheme, context, error))
		return BOOL_FALSE;

	// Iterate VIEWs
	views_iter = kscheme_views_iter(scheme);
	while ((view = kscheme_views_each(&views_iter))) {
		kview_commands_node_t *commands_iter = NULL;
		kcommand_t *command = NULL;
		kview_nspaces_node_t *nspaces_iter = NULL;
		knspace_t *nspace = NULL;

		printf("VIEW: %s\n", kview_name(view));

		// Iterate NSPACEs
		nspaces_iter = kview_nspaces_iter(view);
		while ((nspace = kview_nspaces_each(&nspaces_iter))) {
			const char *view_ref = knspace_view_ref(nspace);
			kview_t *rview = NULL;
			rview = kscheme_find_view(scheme, view_ref);
			if (!view)
				return BOOL_FALSE;
			knspace_set_view(nspace, rview);
			printf("NSPACE: %s\n",
				kview_name(knspace_view(nspace)));
		}

		// Iterate COMMANDs
		commands_iter = kview_commands_iter(view);
		while ((command = kview_commands_each(&commands_iter))) {
			printf("COMMAND: %s\n", kcommand_name(command));
			// ACTIONs
			if (!kscheme_prepare_action_list(scheme,
				kcommand_actions(command), error))
				return BOOL_FALSE;
			// PARAMs
			if (!kscheme_prepare_param_list(scheme,
				kcommand_params(command), error))
				return BOOL_FALSE;
		}
	}

	// Iterate ENTRYs
	entrys_iter = kscheme_entrys_iter(scheme);
	while ((entry = kscheme_entrys_each(&entrys_iter))) {
		if (!kscheme_prepare_entry(scheme, entry, error))
			return BOOL_FALSE;
	}

	return BOOL_TRUE;
}
