#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <faux/list.h>
#include <klish/khelper.h>
#include <klish/kptype.h>


struct kptype_s {
	char *name;
	char *help;
};


// Simple attributes

// Name
KGET_STR(ptype, name);
KSET_STR_ONCE(ptype, name);

// Help
KGET_STR(ptype, help);
KSET_STR(ptype, help);


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
	bool_t retval = BOOL_TRUE;

	// Name [mandatory]
	if (faux_str_is_empty(info->name)) {
		if (error)
			*error = KPTYPE_ERROR_ATTR_NAME;
		retval = BOOL_FALSE;
	} else {
		if (!kptype_set_name(ptype, info->name)) {
			if (error)
				*error = KPTYPE_ERROR_ATTR_NAME;
			retval = BOOL_FALSE;
		}
	}

	// Help
	if (!faux_str_is_empty(info->name)) {
		if (!kptype_set_help(ptype, info->help)) {
			if (error)
				*error = KPTYPE_ERROR_ATTR_HELP;
			retval = BOOL_FALSE;
		}
	}

	return retval;
}
