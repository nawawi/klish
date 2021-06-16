#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <faux/list.h>
#include <klish/khelper.h>
#include <klish/kptype.h>
#include <klish/kparam.h>


struct kparam_s {
	char *name;
	char *help;
	char *ptype_ref; // Text reference to PTYPE
	kptype_t *ptype; // Resolved PARAM's PTYPE
	kparam_mode_e mode;
	faux_list_t *params; // Nested parameters
};

// Simple methods

// Name
KGET_STR(param, name);

// Help
KGET_STR(param, help);
KSET_STR(param, help);

// PTYPE reference (must be resolved later)
KGET_STR(param, ptype_ref);
KSET_STR(param, ptype_ref);

// PTYPE (resolved)
KGET(param, kptype_t *, ptype);
KSET(param, kptype_t *, ptype);

// Mode
KGET(param, kparam_mode_e, mode);
KSET(param, kparam_mode_e, mode);

// PARAM list
KGET(param, faux_list_t *, params);
static KCMP_NESTED(param, param, name);
static KCMP_NESTED_BY_KEY(param, param, name);
KADD_NESTED(param, param);
KFIND_NESTED(param, param);
KNESTED_ITER(param, param);
KNESTED_EACH(param, param);


kparam_t *kparam_new(const char *name)
{
	kparam_t *param = NULL;

	if (faux_str_is_empty(name))
		return NULL;

	param = faux_zmalloc(sizeof(*param));
	assert(param);
	if (!param)
		return NULL;

	// Initialize
	param->name = faux_str_dup(name);
	param->help = NULL;
	param->ptype_ref = NULL;
	param->ptype = NULL;

	param->params = faux_list_new(FAUX_LIST_UNSORTED, FAUX_LIST_UNIQUE,
		kparam_param_compare, kparam_param_kcompare,
		(void (*)(void *))kparam_free);
	assert(param->params);

	return param;
}


void kparam_free(kparam_t *param)
{
	if (!param)
		return;

	faux_str_free(param->name);
	faux_str_free(param->help);
	faux_str_free(param->ptype_ref);
	faux_list_free(param->params);

	faux_free(param);
}
