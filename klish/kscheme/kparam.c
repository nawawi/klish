#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <faux/list.h>
#include <klish/kparam.h>


struct kparam_s {
	bool_t is_static;
	iparam_t info;
	faux_list_t *params; // Nested parameters
};


static int kparam_param_compare(const void *first, const void *second)
{
	const kparam_t *f = (const kparam_t *)first;
	const kparam_t *s = (const kparam_t *)second;

	return strcmp(kparam_name(f), kparam_name(s));
}


static int kparam_param_kcompare(const void *key, const void *list_item)
{
	const char *f = (const char *)key;
	const kparam_t *s = (const kparam_t *)list_item;

	return strcmp(f, kparam_name(s));
}


static kparam_t *kparam_new_internal(iparam_t info, bool_t is_static)
{
	kparam_t *param = NULL;

	param = faux_zmalloc(sizeof(*param));
	assert(param);
	if (!param)
		return NULL;

	// Initialize
	param->is_static = is_static;
	param->info = info;

	param->params = faux_list_new(FAUX_LIST_UNSORTED, FAUX_LIST_UNIQUE,
		kparam_param_compare, kparam_param_kcompare,
		(void (*)(void *))kparam_free);
	assert(param->params);

	return param;
}


kparam_t *kparam_new(iparam_t info)
{
	return kparam_new_internal(info, BOOL_FALSE);
}


kparam_t *kparam_new_static(iparam_t info)
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
	faux_list_free(param->params);

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


bool_t kparam_add_param(kparam_t *param, kparam_t *nested_param)
{
	assert(param);
	if (!param)
		return BOOL_FALSE;
	assert(nested_param);
	if (!nested_param)
		return BOOL_FALSE;

	if (!faux_list_add(param->params, nested_param))
		return BOOL_FALSE;

	return BOOL_TRUE;
}
