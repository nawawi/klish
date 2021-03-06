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
#include <klish/iplugin.h>
#include <klish/kplugin.h>
#include <klish/ksym.h>

#define TAG "PLUGIN"


bool_t kplugin_parse(kplugin_t *plugin, const iplugin_t *info, faux_error_t *error)
{
	bool_t retcode = BOOL_TRUE;

	// ID
	if (!faux_str_is_empty(info->id)) {
		if (!kplugin_set_id(plugin, info->id)) {
			faux_error_add(error, TAG": Illegal 'id' attribute");
			retcode = BOOL_FALSE;
		}
	}

	// File
	if (!faux_str_is_empty(info->file)) {
		if (!kplugin_set_file(plugin, info->file)) {
			faux_error_add(error, TAG": Illegal 'file' attribute");
			retcode = BOOL_FALSE;
		}
	}

	// Global
	if (!faux_str_is_empty(info->global)) {
		bool_t b = BOOL_FALSE;
		if (!faux_conv_str2bool(info->global, &b) ||
			!kplugin_set_global(plugin, b)) {
			faux_error_add(error, TAG": Illegal 'global' attribute");
			retcode = BOOL_FALSE;
		}
	}

	// Conf
	if (!faux_str_is_empty(info->conf)) {
		if (!kplugin_set_conf(plugin, info->conf)) {
			faux_error_add(error, TAG": Illegal 'conf' attribute");
			retcode = BOOL_FALSE;
		}
	}

	return retcode;
}


kplugin_t *kplugin_from_iplugin(iplugin_t *iplugin, faux_error_t *error)
{
	kplugin_t *kplugin = NULL;

	// Name [mandatory]
	if (faux_str_is_empty(iplugin->name))
		return NULL;

	kplugin = kplugin_new(iplugin->name);
	if (!kplugin) {
		faux_error_sprintf(error, TAG" \"%s\": Can't create object",
			iplugin->name);
		return NULL;
	}

	if (!kplugin_parse(kplugin, iplugin, error)) {
		kplugin_free(kplugin);
		return NULL;
	}

	return kplugin;
}
