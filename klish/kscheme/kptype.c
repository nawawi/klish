#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <faux/list.h>
#include <klish/kptype.h>


struct kptype_s {
	bool_t is_static;
	iptype_t info;
};


static kptype_t *kptype_new_internal(iptype_t info, bool_t is_static)
{
	kptype_t *ptype = NULL;

	ptype = faux_zmalloc(sizeof(*ptype));
	assert(ptype);
	if (!ptype)
		return NULL;

	// Initialize
	ptype->is_static = is_static;
	ptype->info = info;

	return ptype;
}


kptype_t *kptype_new(iptype_t info)
{
	return kptype_new_internal(info, BOOL_FALSE);
}


kptype_t *kptype_new_static(iptype_t info)
{
	return kptype_new_internal(info, BOOL_TRUE);
}


void kptype_free(kptype_t *ptype)
{
	if (!ptype)
		return;

	if (!ptype->is_static) {
		faux_str_free(ptype->info.name);
		faux_str_free(ptype->info.help);
	}

	faux_free(ptype);
}


const char *kptype_name(const kptype_t *ptype)
{
	assert(ptype);
	if (!ptype)
		return NULL;

	return ptype->info.name;
}


const char *kptype_help(const kptype_t *ptype)
{
	assert(ptype);
	if (!ptype)
		return NULL;

	return ptype->info.help;
}
