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
	faux_list_t *params; // Nested parameters
};

// Simple methods

// Name
KGET_STR(param, name);
KSET_STR_ONCE(param, name);

// Help
KGET_STR(param, help);
KSET_STR(param, help);

// PTYPE reference (must be resolved later)
KGET_STR(param, ptype_ref);
KSET_STR(param, ptype_ref);

// PTYPE (resolved)
KGET(param, kptype_t *, ptype);
KSET(param, kptype_t *, ptype);

// PARAM list
static KCMP_NESTED(param, param, name);
static KCMP_NESTED_BY_KEY(param, param, name);
KADD_NESTED(param, param);
KFIND_NESTED(param, param);


static kparam_t *kparam_new_empty(void)
{
	kparam_t *param = NULL;

	param = faux_zmalloc(sizeof(*param));
	assert(param);
	if (!param)
		return NULL;

	// Initialize
	param->name = NULL;
	param->help = NULL;
	param->ptype_ref = NULL;
	param->ptype = NULL;

	param->params = faux_list_new(FAUX_LIST_UNSORTED, FAUX_LIST_UNIQUE,
		kparam_param_compare, kparam_param_kcompare,
		(void (*)(void *))kparam_free);
	assert(param->params);

	return param;
}


kparam_t *kparam_new(const iparam_t *info, kparam_error_e *error)
{
	kparam_t *param = NULL;

	param = kparam_new_empty();
	assert(param);
	if (!param) {
		if (error)
			*error = KPARAM_ERROR_ALLOC;
		return NULL;
	}

	if (!info)
		return param;

	if (!kparam_parse(param, info, error)) {
		kparam_free(param);
		return NULL;
	}

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


const char *kparam_strerror(kparam_error_e error)
{
	const char *str = NULL;

	switch (error) {
	case KPARAM_ERROR_OK:
		str = "Ok";
		break;
	case KPARAM_ERROR_INTERNAL:
		str = "Internal error";
		break;
	case KPARAM_ERROR_ALLOC:
		str = "Memory allocation error";
		break;
	case KPARAM_ERROR_ATTR_NAME:
		str = "Illegal 'name' attribute";
		break;
	case KPARAM_ERROR_ATTR_HELP:
		str = "Illegal 'help' attribute";
		break;
	case KPARAM_ERROR_ATTR_PTYPE:
		str = "Illegal 'ptype' attribute";
		break;
	default:
		str = "Unknown error";
		break;
	}

	return str;
}


bool_t kparam_parse(kparam_t *param, const iparam_t *info, kparam_error_e *error)
{
	// Name [mandatory]
	if (faux_str_is_empty(info->name)) {
		if (error)
			*error = KPARAM_ERROR_ATTR_NAME;
		return BOOL_FALSE;
	} else {
		if (!kparam_set_name(param, info->name)) {
			if (error)
				*error = KPARAM_ERROR_ATTR_NAME;
			return BOOL_FALSE;
		}
	}

	// Help
	if (!faux_str_is_empty(info->name)) {
		if (!kparam_set_help(param, info->help)) {
			if (error)
				*error = KPARAM_ERROR_ATTR_HELP;
			return BOOL_FALSE;
		}
	}

	// PTYPE reference
	if (!faux_str_is_empty(info->ptype)) {
		if (!kparam_set_ptype_ref(param, info->ptype)) {
			if (error)
				*error = KPARAM_ERROR_ATTR_PTYPE;
			return BOOL_FALSE;
		}
	}

	return BOOL_TRUE;
}


bool_t kparam_nested_from_iparam(kparam_t *kparam, iparam_t *iparam,
	faux_error_t *error_stack)
{
	bool_t retval = BOOL_TRUE;

	if (!kparam || !iparam) {
		faux_error_add(error_stack,
			kparam_strerror(KPARAM_ERROR_INTERNAL));
		return BOOL_FALSE;
	}

	// Nested PARAM list
	if (iparam->params) {
		iparam_t **p_iparam = NULL;
		for (p_iparam = *iparam->params; *p_iparam; p_iparam++) {
			kparam_t *nkparam = NULL;
			iparam_t *niparam = *p_iparam;

			nkparam = kparam_from_iparam(niparam, error_stack);
			if (!nkparam) {
				retval = BOOL_FALSE;
				continue;
			}
			if (!kparam_add_param(kparam, nkparam)) {
				char *msg = NULL;
				// Search for PARAM duplicates
				if (kparam_find_param(kparam,
					kparam_name(nkparam))) {
					msg = faux_str_sprintf("PARAM: "
						"Can't add duplicate PARAM "
						"\"%s\"",
						kparam_name(nkparam));
				} else {
					msg = faux_str_sprintf("PARAM: "
						"Can't add PARAM \"%s\"",
						kparam_name(nkparam));
				}
				faux_error_add(error_stack, msg);
				faux_str_free(msg);
				kparam_free(nkparam);
				retval = BOOL_FALSE;
				continue;
			}
		}
	}

	if (!retval) {
		char *msg = NULL;
		msg = faux_str_sprintf("PARAM \"%s\": Illegal nested elements",
			kparam_name(kparam));
		faux_error_add(error_stack, msg);
		faux_str_free(msg);
	}

	return retval;
}


kparam_t *kparam_from_iparam(iparam_t *iparam, faux_error_t *error_stack)
{
	kparam_t *kparam = NULL;
	kparam_error_e kparam_error = KPARAM_ERROR_OK;

	kparam = kparam_new(iparam, &kparam_error);
	if (!kparam) {
		char *msg = NULL;
		msg = faux_str_sprintf("PARAM \"%s\": %s",
			iparam->name ? iparam->name : "(null)",
			kparam_strerror(kparam_error));
		faux_error_add(error_stack, msg);
		faux_str_free(msg);
		return NULL;
	}

	// Parse nested elements
	if (!kparam_nested_from_iparam(kparam, iparam, error_stack)) {
		kparam_free(kparam);
		return NULL;
	}

	return kparam;
}
