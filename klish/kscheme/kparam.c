#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <klish/kparam.h>

struct kparam_s {
	bool_t is_static;
	kparam_info_t info;
};


static kparam_t *kparam_new_internal(kparam_info_t info, bool_t is_static)
{
	kparam_t *param = NULL;

	param = faux_zmalloc(sizeof(*param));
	assert(param);
	if (!param)
		return NULL;

	// Initialize
	param->is_static = is_static;
	param->info = info;

	return param;
}


kparam_t *kparam_new(kparam_info_t info)
{
	return kparam_new_internal(info, BOOL_FALSE);
}


kparam_t *kparam_new_static(kparam_info_t info)
{
	return kparam_new_internal(info, BOOL_TRUE);
}


void kparam_free(kparam_t *param)
{
	if (!param)
		return;

	if (!param->is_static) {
		faux_str_free(param->info.name);
		faux_str_free(param->info.help);
		faux_str_free(param->info.ptype);
	}

	faux_free(param);
}


const char *kparam_name(const kparam_t *param)
{
	assert(param);
	if (!param)
		return NULL;

	return param->info.name;
}


const char *kparam_help(const kparam_t *param)
{
	assert(param);
	if (!param)
		return NULL;

	return param->info.help;
}


const char *kparam_ptype_str(const kparam_t *param)
{
	assert(param);
	if (!param)
		return NULL;

	return param->info.ptype;
}
