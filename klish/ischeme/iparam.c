#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <faux/list.h>
#include <klish/khelper.h>
#include <klish/iparam.h>
#include <klish/kptype.h>
#include <klish/kparam.h>

#define TAG "PARAM"

bool_t kparam_parse(kparam_t *param, const iparam_t *info, faux_error_t *error)
{
	bool_t retcode = BOOL_TRUE;

	// Help
	if (!faux_str_is_empty(info->help)) {
		if (!kparam_set_help(param, info->help)) {
			faux_error_add(error, TAG": Illegal 'help' attribute");
			retcode = BOOL_FALSE;
		}
	}

	// PTYPE reference
	if (!faux_str_is_empty(info->ptype)) {
		if (!kparam_set_ptype_ref(param, info->ptype)) {
			faux_error_add(error, TAG": Illegal 'ptype' attribute");
			retcode = BOOL_FALSE;
		}
	}

	return retcode;
}


bool_t kparam_nested_from_iparam(kparam_t *kparam, iparam_t *iparam,
	faux_error_t *error)
{
	bool_t retval = BOOL_TRUE;

	if (!kparam || !iparam) {
		faux_error_add(error, TAG": Internal error");
		return BOOL_FALSE;
	}

	// Nested PARAM list
	if (iparam->params) {
		iparam_t **p_iparam = NULL;
		for (p_iparam = *iparam->params; *p_iparam; p_iparam++) {
			kparam_t *nkparam = NULL;
			iparam_t *niparam = *p_iparam;

			nkparam = kparam_from_iparam(niparam, error);
			if (!nkparam) {
				retval = BOOL_FALSE;
				continue;
			}
			if (!kparam_add_param(kparam, nkparam)) {
				// Search for PARAM duplicates
				if (kparam_find_param(kparam,
					kparam_name(nkparam))) {
					faux_error_sprintf(error,
						TAG": Can't add duplicate PARAM "
						"\"%s\"", kparam_name(nkparam));
				} else {
					faux_error_sprintf(error,
						TAG": Can't add PARAM \"%s\"",
						kparam_name(nkparam));
				}
				kparam_free(nkparam);
				retval = BOOL_FALSE;
				continue;
			}
		}
	}

	if (!retval)
		faux_error_sprintf(error, TAG" \"%s\": Illegal nested elements",
			kparam_name(kparam));

	return retval;
}


kparam_t *kparam_from_iparam(iparam_t *iparam, faux_error_t *error)
{
	kparam_t *kparam = NULL;

	// Name [mandatory]
	if (faux_str_is_empty(iparam->name)) {
		faux_error_add(error, TAG": Empty 'name' attribute");
		return NULL;
	}

	kparam = kparam_new(iparam->name);
	if (!kparam) {
		faux_error_sprintf(error, TAG" \"%s\": Can't create object",
			iparam->name);
		return NULL;
	}

	if (!kparam_parse(kparam, iparam, error)) {
		kparam_free(kparam);
		return NULL;
	}

	// Parse nested elements
	if (!kparam_nested_from_iparam(kparam, iparam, error)) {
		kparam_free(kparam);
		return NULL;
	}

	return kparam;
}


char *iparam_to_text(const iparam_t *iparam, int level)
{
	char *str = NULL;
	char *tmp = NULL;

	tmp = faux_str_sprintf("%*cPARAM {\n", level, ' ');
	faux_str_cat(&str, tmp);
	faux_str_free(tmp);

	attr2ctext(&str, "name", iparam->name, level + 1);
	attr2ctext(&str, "help", iparam->help, level + 1);
	attr2ctext(&str, "ptype", iparam->ptype, level + 1);

	// PARAM list
	if (iparam->params) {
		iparam_t **p_iparam = NULL;

		tmp = faux_str_sprintf("\n%*cPARAM_LIST\n\n", level + 1, ' ');
		faux_str_cat(&str, tmp);
		faux_str_free(tmp);

		for (p_iparam = *iparam->params; *p_iparam; p_iparam++) {
			iparam_t *niparam = *p_iparam;

			tmp = iparam_to_text(niparam, level + 2);
			faux_str_cat(&str, tmp);
			faux_str_free(tmp);
		}

		tmp = faux_str_sprintf("%*cEND_PARAM_LIST,\n", level + 1, ' ');
		faux_str_cat(&str, tmp);
		faux_str_free(tmp);
	}

	tmp = faux_str_sprintf("%*c},\n\n", level, ' ');
	faux_str_cat(&str, tmp);
	faux_str_free(tmp);

	return str;
}
