#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <faux/list.h>
#include <faux/error.h>
#include <klish/khelper.h>
#include <klish/iparam.h>
#include <klish/iaction.h>
#include <klish/icommand.h>
#include <klish/kparam.h>
#include <klish/kaction.h>
#include <klish/kcommand.h>

#define TAG "COMMAND"

bool_t icommand_parse(const icommand_t *info, kcommand_t *command,
	faux_error_t *error)
{
	bool_t retcode = BOOL_TRUE;

	if (!info)
		return BOOL_FALSE;
	if (!command)
		return BOOL_FALSE;

	// Help
	if (!faux_str_is_empty(info->help)) {
		if (!kcommand_set_help(command, info->help)) {
			faux_error_add(error, TAG": Illegal 'help' attribute");
			retcode = BOOL_FALSE;
		}
	}

	return retcode;
}


bool_t icommand_parse_nested(const icommand_t *icommand, kcommand_t *kcommand,
	faux_error_t *error)
{
	bool_t retval = BOOL_TRUE;

	if (!kcommand || !icommand) {
		faux_error_add(error, TAG": Internal error");
		return BOOL_FALSE;
	}


	// PARAM list
	if (icommand->params) {
		iparam_t **p_iparam = NULL;
		for (p_iparam = *icommand->params; *p_iparam; p_iparam++) {
			kparam_t *kparam = NULL;
			iparam_t *iparam = *p_iparam;

			kparam = iparam_load(iparam, error);
			if (!kparam) {
				retval = BOOL_FALSE;
				continue;
			}
			if (!kcommand_add_params(kcommand, kparam)) {
				// Search for PARAM duplicates
				if (kcommand_find_param(kcommand,
					kparam_name(kparam))) {
					faux_error_sprintf(error,
						TAG": Can't add duplicate PARAM "
						"\"%s\"", kparam_name(kparam));
				} else {
					faux_error_sprintf(error,
						TAG": Can't add PARAM \"%s\"",
						kparam_name(kparam));
				}
				kparam_free(kparam);
				retval = BOOL_FALSE;
				continue;
			}
		}
	}

	// ACTION list
	if (icommand->actions) {
		iaction_t **p_iaction = NULL;
		for (p_iaction = *icommand->actions; *p_iaction; p_iaction++) {
			kaction_t *kaction = NULL;
			iaction_t *iaction = *p_iaction;

			kaction = iaction_load(iaction, error);
			if (!kaction) {
				retval = BOOL_FALSE;
				continue;
			}
			if (!kcommand_add_actions(kcommand, kaction)) {
				faux_error_sprintf(error,
					TAG": Can't add ACTION #%d",
					kcommand_actions_len(kcommand) + 1);
				kaction_free(kaction);
				retval = BOOL_FALSE;
				continue;
			}
		}
	}

	if (!retval)
		faux_error_sprintf(error, TAG" \"%s\": Illegal nested elements",
			kcommand_name(kcommand));

	return retval;
}


kcommand_t *icommand_load(icommand_t *icommand, faux_error_t *error)
{
	kcommand_t *kcommand = NULL;

	// Name [mandatory]
	if (faux_str_is_empty(icommand->name)) {
		faux_error_add(error, TAG": Empty 'name' attribute");
		return NULL;
	}

	kcommand = kcommand_new(icommand->name);
	if (!kcommand) {
		faux_error_sprintf(error, TAG" \"%s\": Can't create object",
			icommand->name);
		return NULL;
	}

	if (!icommand_parse(icommand, kcommand, error)) {
		kcommand_free(kcommand);
		return NULL;
	}

	// Parse nested elements
	if (!icommand_parse_nested(icommand, kcommand, error)) {
		kcommand_free(kcommand);
		return NULL;
	}

	return kcommand;
}


char *icommand_deploy(const kcommand_t *kcommand, int level)
{
	char *str = NULL;
	char *tmp = NULL;
	kcommand_params_node_t *params_iter = NULL;
	kcommand_actions_node_t *actions_iter = NULL;

	if (!kcommand)
		return NULL;

	tmp = faux_str_sprintf("%*cCOMMAND {\n", level, ' ');
	faux_str_cat(&str, tmp);
	faux_str_free(tmp);

	attr2ctext(&str, "name", kcommand_name(kcommand), level + 1);
	attr2ctext(&str, "help", kcommand_help(kcommand), level + 1);

	// PARAM list
	params_iter = kcommand_params_iter(kcommand);
	if (params_iter) {
		kparam_t *param = NULL;

		tmp = faux_str_sprintf("\n%*cPARAM_LIST\n\n", level + 1, ' ');
		faux_str_cat(&str, tmp);
		faux_str_free(tmp);

		while ((param = kcommand_params_each(&params_iter))) {
			tmp = iparam_deploy(param, level + 2);
			faux_str_cat(&str, tmp);
			faux_str_free(tmp);
		}

		tmp = faux_str_sprintf("%*cEND_PARAM_LIST,\n", level + 1, ' ');
		faux_str_cat(&str, tmp);
		faux_str_free(tmp);
	}

	// ACTION list
	actions_iter = kcommand_actions_iter(kcommand);
	if (actions_iter) {
		kaction_t *action = NULL;

		tmp = faux_str_sprintf("\n%*cACTION_LIST\n\n", level + 1, ' ');
		faux_str_cat(&str, tmp);
		faux_str_free(tmp);

		while ((action = kcommand_actions_each(&actions_iter))) {
			tmp = iaction_deploy(action, level + 2);
			faux_str_cat(&str, tmp);
			faux_str_free(tmp);
		}

		tmp = faux_str_sprintf("%*cEND_ACTION_LIST,\n", level + 1, ' ');
		faux_str_cat(&str, tmp);
		faux_str_free(tmp);
	}

	tmp = faux_str_sprintf("%*c},\n\n", level, ' ');
	faux_str_cat(&str, tmp);
	faux_str_free(tmp);

	return str;
}
