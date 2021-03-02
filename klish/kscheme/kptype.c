#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <faux/list.h>
#include <faux/error.h>
#include <klish/khelper.h>
#include <klish/kptype.h>
#include <klish/kaction.h>


struct kptype_s {
	char *name;
	char *help;
	faux_list_t *actions;
};


// Simple methods

// Name
KGET_STR(ptype, name);
KSET_STR_ONCE(ptype, name);

// Help
KGET_STR(ptype, help);
KSET_STR(ptype, help);

// ACTION list
KADD_NESTED(ptype, action);


static kptype_t *kptype_new_empty(void)
{
	kptype_t *ptype = NULL;

	ptype = faux_zmalloc(sizeof(*ptype));
	assert(ptype);
	if (!ptype)
		return NULL;

	// Initialize
	ptype->name = NULL;
	ptype->help = NULL;

	// ACTION list
	ptype->actions = faux_list_new(FAUX_LIST_UNSORTED, FAUX_LIST_NONUNIQUE,
		NULL, NULL, (void (*)(void *))kaction_free);
	assert(ptype->actions);

	return ptype;
}


kptype_t *kptype_new(const iptype_t *info, kptype_error_e *error)
{
	kptype_t *ptype = NULL;

	ptype = kptype_new_empty();
	assert(ptype);
	if (!ptype) {
		if (error)
			*error = KPTYPE_ERROR_ALLOC;
		return NULL;
	}

	if (!info)
		return ptype;

	if (!kptype_parse(ptype, info, error)) {
		kptype_free(ptype);
		return NULL;
	}

	return ptype;
}


void kptype_free(kptype_t *ptype)
{
	if (!ptype)
		return;

	faux_str_free(ptype->name);
	faux_str_free(ptype->help);
	faux_list_free(ptype->actions);

	faux_free(ptype);
}


const char *kptype_strerror(kptype_error_e error)
{
	const char *str = NULL;

	switch (error) {
	case KPTYPE_ERROR_OK:
		str = "Ok";
		break;
	case KPTYPE_ERROR_INTERNAL:
		str = "Internal error";
		break;
	case KPTYPE_ERROR_ALLOC:
		str = "Memory allocation error";
		break;
	case KPTYPE_ERROR_ATTR_NAME:
		str = "Illegal 'name' attribute";
		break;
	case KPTYPE_ERROR_ATTR_HELP:
		str = "Illegal 'help' attribute";
		break;
	default:
		str = "Unknown error";
		break;
	}

	return str;
}


bool_t kptype_parse(kptype_t *ptype, const iptype_t *info, kptype_error_e *error)
{
	// Name [mandatory]
	if (faux_str_is_empty(info->name)) {
		if (error)
			*error = KPTYPE_ERROR_ATTR_NAME;
		return BOOL_FALSE;
	} else {
		if (!kptype_set_name(ptype, info->name)) {
			if (error)
				*error = KPTYPE_ERROR_ATTR_NAME;
			return BOOL_FALSE;
		}
	}

	// Help
	if (!faux_str_is_empty(info->name)) {
		if (!kptype_set_help(ptype, info->help)) {
			if (error)
				*error = KPTYPE_ERROR_ATTR_HELP;
			return BOOL_FALSE;
		}
	}

	return BOOL_TRUE;
}


bool_t kptype_nested_from_iptype(kptype_t *kptype, iptype_t *iptype,
	faux_error_t *error_stack)
{
	bool_t retval = BOOL_TRUE;

	if (!kptype || !iptype) {
		faux_error_add(error_stack,
			kptype_strerror(KPTYPE_ERROR_INTERNAL));
		return BOOL_FALSE;
	}

	// ACTION list
	if (iptype->actions) {
		iaction_t **p_iaction = NULL;
		for (p_iaction = *iptype->actions; *p_iaction; p_iaction++) {
			kaction_t *kaction = NULL;
			iaction_t *iaction = *p_iaction;

			kaction = kaction_from_iaction(iaction, error_stack);
			if (!kaction) {
				retval = BOOL_FALSE;
				continue;
			}
			if (!kptype_add_action(kptype, kaction)) {
				char *msg = NULL;
				msg = faux_str_sprintf("PTYPE: "
					"Can't add ACTION #%d",
					faux_list_len(kptype->actions) + 1);
				faux_error_add(error_stack, msg);
				faux_str_free(msg);
				kaction_free(kaction);
				retval = BOOL_FALSE;
				continue;
			}
		}
	}

	if (!retval) {
		char *msg = NULL;
		msg = faux_str_sprintf("PTYPE \"%s\": Illegal nested elements",
			kptype_name(kptype));
		faux_error_add(error_stack, msg);
		faux_str_free(msg);
	}

	return retval;
}


kptype_t *kptype_from_iptype(iptype_t *iptype, faux_error_t *error_stack)
{
	kptype_t *kptype = NULL;
	kptype_error_e kptype_error = KPTYPE_ERROR_OK;

	kptype = kptype_new(iptype, &kptype_error);
	if (!kptype) {
		char *msg = NULL;
		msg = faux_str_sprintf("PTYPE \"%s\": %s",
			iptype->name ? iptype->name : "(null)",
			kptype_strerror(kptype_error));
		faux_error_add(error_stack, msg);
		faux_str_free(msg);
		return NULL;
	}

	// Parse nested elements
	if (!kptype_nested_from_iptype(kptype, iptype, error_stack)) {
		kptype_free(kptype);
		return NULL;
	}

	return kptype;
}
