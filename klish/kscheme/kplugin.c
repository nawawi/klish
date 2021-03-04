#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <faux/list.h>
#include <faux/conv.h>
#include <faux/error.h>
#include <klish/khelper.h>
#include <klish/kplugin.h>
#include <klish/ksym.h>


struct kplugin_s {
	char *name;
	char *id;
	char *file;
	bool_t global;
	char *conf;
	uint8_t major;
	uint8_t minor;
	faux_list_t *syms;
};


// Simple methods

// Name
KGET_STR(plugin, name);
KSET_STR_ONCE(plugin, name);

// ID
KGET_STR(plugin, id);
KSET_STR(plugin, id);

// File
KGET_STR(plugin, file);
KSET_STR(plugin, file);

// Global
KGET_BOOL(plugin, global);
KSET_BOOL(plugin, global);

// Conf
KGET_STR(plugin, conf);
KSET_STR(plugin, conf);

// Version major number
KGET(plugin, uint8_t, major);
KSET(plugin, uint8_t, major);

// Version minor number
KGET(plugin, uint8_t, minor);
KSET(plugin, uint8_t, minor);

// COMMAND list
static KCMP_NESTED(plugin, sym, name);
static KCMP_NESTED_BY_KEY(plugin, sym, name);
KADD_NESTED(plugin, sym);
KFIND_NESTED(plugin, sym);


static kplugin_t *kplugin_new_empty(void)
{
	kplugin_t *plugin = NULL;

	plugin = faux_zmalloc(sizeof(*plugin));
	assert(plugin);
	if (!plugin)
		return NULL;

	// Initialize
	plugin->name = NULL;
	plugin->id = NULL;
	plugin->file = NULL;
	plugin->global = BOOL_FALSE;
	plugin->conf = NULL;
	plugin->major = 0;
	plugin->minor = 0;

	// SYM list
	plugin->syms = faux_list_new(FAUX_LIST_SORTED, FAUX_LIST_UNIQUE,
		kplugin_sym_compare, kplugin_sym_kcompare,
		(void (*)(void *))ksym_free);
	assert(plugin->syms);

	return plugin;
}


kplugin_t *kplugin_new(const iplugin_t *info, kplugin_error_e *error)
{
	kplugin_t *plugin = NULL;

	plugin = kplugin_new_empty();
	assert(plugin);
	if (!plugin) {
		if (error)
			*error = KPLUGIN_ERROR_ALLOC;
		return NULL;
	}

	if (!info)
		return plugin;

	if (!kplugin_parse(plugin, info, error)) {
		kplugin_free(plugin);
		return NULL;
	}

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

	faux_free(plugin);
}


const char *kplugin_strerror(kplugin_error_e error)
{
	const char *str = NULL;

	switch (error) {
	case KPLUGIN_ERROR_OK:
		str = "Ok";
		break;
	case KPLUGIN_ERROR_INTERNAL:
		str = "Internal error";
		break;
	case KPLUGIN_ERROR_ALLOC:
		str = "Memory allocation error";
		break;
	case KPLUGIN_ERROR_ATTR_NAME:
		str = "Illegal 'name' attribute";
		break;
	case KPLUGIN_ERROR_ATTR_ID:
		str = "Illegal 'id' attribute";
		break;
	case KPLUGIN_ERROR_ATTR_FILE:
		str = "Illegal 'file' attribute";
		break;
	case KPLUGIN_ERROR_ATTR_GLOBAL:
		str = "Illegal 'global' attribute";
		break;
	case KPLUGIN_ERROR_ATTR_CONF:
		str = "Illegal conf";
		break;
	default:
		str = "Unknown error";
		break;
	}

	return str;
}


bool_t kplugin_parse(kplugin_t *plugin, const iplugin_t *info, kplugin_error_e *error)
{
	// Name [mandatory]
	if (faux_str_is_empty(info->name)) {
		if (error)
			*error = KPLUGIN_ERROR_ATTR_NAME;
		return BOOL_FALSE;
	} else {
		if (!kplugin_set_name(plugin, info->name)) {
			if (error)
				*error = KPLUGIN_ERROR_ATTR_NAME;
			return BOOL_FALSE;
		}
	}

	// ID
	if (!faux_str_is_empty(info->id)) {
		if (!kplugin_set_id(plugin, info->id)) {
			if (error)
				*error = KPLUGIN_ERROR_ATTR_ID;
			return BOOL_FALSE;
		}
	}

	// File
	if (!faux_str_is_empty(info->file)) {
		if (!kplugin_set_file(plugin, info->file)) {
			if (error)
				*error = KPLUGIN_ERROR_ATTR_FILE;
			return BOOL_FALSE;
		}
	}

	// Global
	if (!faux_str_is_empty(info->global)) {
		bool_t b = BOOL_FALSE;
		if (!faux_conv_str2bool(info->global, &b) ||
			!kplugin_set_global(plugin, b)) {
			if (error)
				*error = KPLUGIN_ERROR_ATTR_GLOBAL;
			return BOOL_FALSE;
		}
	}

	// Conf
	if (!faux_str_is_empty(info->conf)) {
		if (!kplugin_set_conf(plugin, info->conf)) {
			if (error)
				*error = KPLUGIN_ERROR_ATTR_CONF;
			return BOOL_FALSE;
		}
	}

	return BOOL_TRUE;
}


kplugin_t *kplugin_from_iplugin(iplugin_t *iplugin, faux_error_t *error_stack)
{
	kplugin_t *kplugin = NULL;
	kplugin_error_e kplugin_error = KPLUGIN_ERROR_OK;

	kplugin = kplugin_new(iplugin, &kplugin_error);
	if (!kplugin) {
		faux_error_sprintf(error_stack, "PLUGIN \"%s\": %s",
			iplugin->name ? iplugin->name : "(null)",
			kplugin_strerror(kplugin_error));
		return NULL;
	}

	return kplugin;
}
