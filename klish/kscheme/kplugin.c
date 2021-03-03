#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <faux/list.h>
#include <faux/conv.h>
#include <faux/error.h>
#include <klish/khelper.h>
#include <klish/kplugin.h>


struct kplugin_s {
	char *name;
	char *alias;
	char *file;
	bool_t global;
	char *script;
};


// Simple methods

// Name
KGET_STR(plugin, name);
KSET_STR_ONCE(plugin, name);

// Alias
KGET_STR(plugin, alias);
KSET_STR(plugin, alias);

// Alias
KGET_STR(plugin, file);
KSET_STR(plugin, file);

// Global
KGET_BOOL(plugin, global);
KSET_BOOL(plugin, global);

// Script
KGET_STR(plugin, script);
KSET_STR(plugin, script);


static kplugin_t *kplugin_new_empty(void)
{
	kplugin_t *plugin = NULL;

	plugin = faux_zmalloc(sizeof(*plugin));
	assert(plugin);
	if (!plugin)
		return NULL;

	// Initialize
	plugin->name = NULL;
	plugin->alias = NULL;
	plugin->file = NULL;
	plugin->global = BOOL_FALSE;
	plugin->script = NULL;

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
	faux_str_free(plugin->alias);
	faux_str_free(plugin->file);
	faux_str_free(plugin->script);

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
	case KPLUGIN_ERROR_ATTR_ALIAS:
		str = "Illegal 'alias' attribute";
		break;
	case KPLUGIN_ERROR_ATTR_FILE:
		str = "Illegal 'file' attribute";
		break;
	case KPLUGIN_ERROR_ATTR_GLOBAL:
		str = "Illegal 'global' attribute";
		break;
	case KPLUGIN_ERROR_SCRIPT:
		str = "Illegal script";
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

	// Alias
	if (!faux_str_is_empty(info->alias)) {
		if (!kplugin_set_alias(plugin, info->alias)) {
			if (error)
				*error = KPLUGIN_ERROR_ATTR_ALIAS;
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

	// Script
	if (!faux_str_is_empty(info->script)) {
		if (!kplugin_set_script(plugin, info->script)) {
			if (error)
				*error = KPLUGIN_ERROR_SCRIPT;
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
