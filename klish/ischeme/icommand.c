#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <faux/list.h>
#include <faux/error.h>
#include <klish/khelper.h>
#include <klish/icommand.h>
#include <klish/kparam.h>
#include <klish/kaction.h>
#include <klish/kcommand.h>

#define TAG "COMMAND"

bool_t kcommand_parse(kcommand_t *command, const icommand_t *info,
	faux_error_t *error)
{
	bool_t retcode = BOOL_TRUE;

	// Help
	if (!faux_str_is_empty(info->help)) {
		if (!kcommand_set_help(command, info->help)) {
			faux_error_add(error, TAG": Illegal 'help' attribute");
			retcode = BOOL_FALSE;
		}
	}

	return retcode;
}


bool_t kcommand_nested_from_icommand(kcommand_t *kcommand, icommand_t *icommand,
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

			kparam = kparam_from_iparam(iparam, error);
			if (!kparam) {
				retval = BOOL_FALSE;
				continue;
			}
			if (!kcommand_add_param(kcommand, kparam)) {
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

			kaction = kaction_from_iaction(iaction, error);
			if (!kaction) {
				retval = BOOL_FALSE;
				continue;
			}
			if (!kcommand_add_action(kcommand, kaction)) {
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


kcommand_t *kcommand_from_icommand(icommand_t *icommand, faux_error_t *error)
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

	if (!kcommand_parse(kcommand, icommand, error)) {
		kcommand_free(kcommand);
		return NULL;
	}

	// Parse nested elements
	if (!kcommand_nested_from_icommand(kcommand, icommand, error)) {
		kcommand_free(kcommand);
		return NULL;
	}

	return kcommand;
}


char *icommand_to_text(const icommand_t *icommand, int level)
{
	char *str = NULL;
	char *tmp = NULL;

	tmp = faux_str_sprintf("%*cCOMMAND {\n", level, ' ');
	faux_str_cat(&str, tmp);
	faux_str_free(tmp);

	attr2ctext(&str, "name", icommand->name, level + 1);
	attr2ctext(&str, "help", icommand->help, level + 1);

	// PARAM list
	if (icommand->params) {
		iparam_t **p_iparam = NULL;

		tmp = faux_str_sprintf("\n%*cPARAM_LIST\n\n", level + 1, ' ');
		faux_str_cat(&str, tmp);
		faux_str_free(tmp);

		for (p_iparam = *icommand->params; *p_iparam; p_iparam++) {
			iparam_t *iparam = *p_iparam;

			tmp = iparam_to_text(iparam, level + 2);
			faux_str_cat(&str, tmp);
			faux_str_free(tmp);
		}

		tmp = faux_str_sprintf("%*cEND_PARAM_LIST,\n", level + 1, ' ');
		faux_str_cat(&str, tmp);
		faux_str_free(tmp);
	}

	// ACTION list
	if (icommand->actions) {
		iaction_t **p_iaction = NULL;

		tmp = faux_str_sprintf("\n%*cACTION_LIST\n\n", level + 1, ' ');
		faux_str_cat(&str, tmp);
		faux_str_free(tmp);

		for (p_iaction = *icommand->actions; *p_iaction; p_iaction++) {
			iaction_t *iaction = *p_iaction;

			tmp = iaction_to_text(iaction, level + 2);
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
