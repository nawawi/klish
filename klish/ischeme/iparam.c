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

bool_t iparam_parse(const iparam_t *info, kparam_t *param, faux_error_t *error)
{
	bool_t retcode = BOOL_TRUE;

	if (!info)
		return BOOL_FALSE;
	if (!param)
		return BOOL_FALSE;

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

	// Mode
	if (!faux_str_is_empty(info->mode)) {
		kparam_mode_e mode = KPARAM_NONE;
		if (!faux_str_casecmp(info->mode, "common"))
			mode = KPARAM_COMMON;
		else if (!faux_str_casecmp(info->mode, "switch"))
			mode = KPARAM_SWITCH;
		else if (!faux_str_casecmp(info->mode, "subcommand"))
			mode = KPARAM_SUBCOMMAND;
		else if (!faux_str_casecmp(info->mode, "multi"))
			mode = KPARAM_MULTI;
		if ((KPARAM_NONE == mode) || !kparam_set_mode(param, mode)) {
			faux_error_add(error, TAG": Illegal 'mode' attribute");
			retcode = BOOL_FALSE;
		}
	}

	return retcode;
}


bool_t iparam_parse_nested(const iparam_t *iparam, kparam_t *kparam,
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

			nkparam = iparam_load(niparam, error);
			if (!nkparam) {
				retval = BOOL_FALSE;
				continue;
			}
			if (!kparam_add_params(kparam, nkparam)) {
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


kparam_t *iparam_load(const iparam_t *iparam, faux_error_t *error)
{
	kparam_t *kparam = NULL;

	if (!iparam)
		return NULL;

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

	if (!iparam_parse(iparam, kparam, error)) {
		kparam_free(kparam);
		return NULL;
	}

	// Parse nested elements
	if (!iparam_parse_nested(iparam, kparam, error)) {
		kparam_free(kparam);
		return NULL;
	}

	return kparam;
}


char *iparam_deploy(const kparam_t *kparam, int level)
{
	char *str = NULL;
	char *tmp = NULL;
	char *mode = NULL;
	kparam_params_node_t *params_iter = NULL;

	tmp = faux_str_sprintf("%*cPARAM {\n", level, ' ');
	faux_str_cat(&str, tmp);
	faux_str_free(tmp);

	attr2ctext(&str, "name", kparam_name(kparam), level + 1);
	attr2ctext(&str, "help", kparam_help(kparam), level + 1);
	attr2ctext(&str, "ptype", kparam_ptype_ref(kparam), level + 1);

	// Mode
	switch (kparam_mode(kparam)) {
	case KPARAM_COMMON:
		mode = "common";
		break;
	case KPARAM_SWITCH:
		mode = "switch";
		break;
	case KPARAM_SUBCOMMAND:
		mode = "subcommand";
		break;
	case KPARAM_MULTI:
		mode = "multi";
		break;
	default:
		mode = NULL;
	}
	attr2ctext(&str, "mode", mode, level + 1);

	// PARAM list
	params_iter = kparam_params_iter(kparam);
	if (params_iter) {
		kparam_t *nparam = NULL;

		tmp = faux_str_sprintf("\n%*cPARAM_LIST\n\n", level + 1, ' ');
		faux_str_cat(&str, tmp);
		faux_str_free(tmp);

		while ((nparam = kparam_params_each(&params_iter))) {
			tmp = iparam_deploy(nparam, level + 2);
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
