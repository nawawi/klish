#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <faux/list.h>
#include <faux/error.h>
#include <klish/khelper.h>
#include <klish/kparam.h>
#include <klish/kaction.h>
#include <klish/kcommand.h>


struct kcommand_s {
	char *name;
	char *help;
	faux_list_t *params;
	faux_list_t *actions;
};

// Simple attributes

// Name
KGET_STR(command, name);
KSET_STR_ONCE(command, name);

// Help
KGET_STR(command, help);
KSET_STR(command, help);

// PARAM list
static KCMP_NESTED(command, param, name);
static KCMP_NESTED_BY_KEY(command, param, name);
KADD_NESTED(command, param);
KFIND_NESTED(command, param);

// ACTION list
KADD_NESTED(command, action);


static kcommand_t *kcommand_new_empty(void)
{
	kcommand_t *command = NULL;

	command = faux_zmalloc(sizeof(*command));
	assert(command);
	if (!command)
		return NULL;

	// Initialize
	command->name = NULL;
	command->help = NULL;

	// PARAM list
	command->params = faux_list_new(FAUX_LIST_UNSORTED, FAUX_LIST_UNIQUE,
		kcommand_param_compare, kcommand_param_kcompare,
		(void (*)(void *))kparam_free);
	assert(command->params);

	// ACTION list
	command->actions = faux_list_new(FAUX_LIST_UNSORTED, FAUX_LIST_NONUNIQUE,
		NULL, NULL, (void (*)(void *))kaction_free);
	assert(command->actions);

	return command;
}


kcommand_t *kcommand_new(const icommand_t *info, kcommand_error_e *error)
{
	kcommand_t *command = NULL;

	command = kcommand_new_empty();
	assert(command);
	if (!command) {
		if (error)
			*error = KCOMMAND_ERROR_ALLOC;
		return NULL;
	}

	if (!info)
		return command;

	if (!kcommand_parse(command, info, error)) {
		kcommand_free(command);
		return NULL;
	}

	return command;
}


void kcommand_free(kcommand_t *command)
{
	if (!command)
		return;

	faux_str_free(command->name);
	faux_str_free(command->help);
	faux_list_free(command->params);
	faux_list_free(command->actions);

	faux_free(command);
}


const char *kcommand_strerror(kcommand_error_e error)
{
	const char *str = NULL;

	switch (error) {
	case KCOMMAND_ERROR_OK:
		str = "Ok";
		break;
	case KCOMMAND_ERROR_INTERNAL:
		str = "Internal error";
		break;
	case KCOMMAND_ERROR_ALLOC:
		str = "Memory allocation error";
		break;
	case KCOMMAND_ERROR_ATTR_NAME:
		str = "Illegal 'name' attribute";
		break;
	case KCOMMAND_ERROR_ATTR_HELP:
		str = "Illegal 'help' attribute";
		break;
	default:
		str = "Unknown error";
		break;
	}

	return str;
}


bool_t kcommand_parse(kcommand_t *command, const icommand_t *info, kcommand_error_e *error)
{
	// Name [mandatory]
	if (faux_str_is_empty(info->name)) {
		if (error)
			*error = KCOMMAND_ERROR_ATTR_NAME;
		return BOOL_FALSE;
	} else {
		if (!kcommand_set_name(command, info->name)) {
			if (error)
				*error = KCOMMAND_ERROR_ATTR_NAME;
			return BOOL_FALSE;
		}
	}

	// Help
	if (!faux_str_is_empty(info->name)) {
		if (!kcommand_set_help(command, info->help)) {
			if (error)
				*error = KCOMMAND_ERROR_ATTR_HELP;
			return BOOL_FALSE;
		}
	}

	return BOOL_TRUE;
}


bool_t kcommand_nested_from_icommand(kcommand_t *kcommand, icommand_t *icommand,
	faux_error_t *error_stack)
{
	bool_t retval = BOOL_TRUE;

	if (!kcommand || !icommand) {
		faux_error_add(error_stack,
			kcommand_strerror(KCOMMAND_ERROR_INTERNAL));
		return BOOL_FALSE;
	}


	// PARAM list
	if (icommand->params) {
		iparam_t **p_iparam = NULL;
		for (p_iparam = *icommand->params; *p_iparam; p_iparam++) {
			kparam_t *kparam = NULL;
			iparam_t *iparam = *p_iparam;

			kparam = kparam_from_iparam(iparam, error_stack);
			if (!kparam) {
				retval = BOOL_FALSE;
				continue;
			}
			if (!kcommand_add_param(kcommand, kparam)) {
				// Search for PARAM duplicates
				if (kcommand_find_param(kcommand,
					kparam_name(kparam))) {
					faux_error_sprintf(error_stack,
						"COMMAND: "
						"Can't add duplicate PARAM "
						"\"%s\"",
						kparam_name(kparam));
				} else {
					faux_error_sprintf(error_stack,
						"COMMAND: "
						"Can't add PARAM \"%s\"",
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

			kaction = kaction_from_iaction(iaction, error_stack);
			if (!kaction) {
				retval = BOOL_FALSE;
				continue;
			}
			if (!kcommand_add_action(kcommand, kaction)) {
				faux_error_sprintf(error_stack, "COMMAND: "
					"Can't add ACTION #%d",
					faux_list_len(kcommand->actions) + 1);
				kaction_free(kaction);
				retval = BOOL_FALSE;
				continue;
			}
		}
	}

	if (!retval)
		faux_error_sprintf(error_stack,
			"COMMAND \"%s\": Illegal nested elements",
			kcommand_name(kcommand));

	return retval;
}


kcommand_t *kcommand_from_icommand(icommand_t *icommand, faux_error_t *error_stack)
{
	kcommand_t *kcommand = NULL;
	kcommand_error_e kcommand_error = KCOMMAND_ERROR_OK;

	kcommand = kcommand_new(icommand, &kcommand_error);
	if (!kcommand) {
		faux_error_sprintf(error_stack, "COMMAND \"%s\": %s",
			icommand->name ? icommand->name : "(null)",
			kcommand_strerror(kcommand_error));
		return NULL;
	}

	// Parse nested elements
	if (!kcommand_nested_from_icommand(kcommand, icommand, error_stack)) {
		kcommand_free(kcommand);
		return NULL;
	}

	return kcommand;
}
