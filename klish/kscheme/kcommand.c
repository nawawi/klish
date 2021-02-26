#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <faux/list.h>
#include <faux/error.h>
#include <klish/khelper.h>
#include <klish/kparam.h>
#include <klish/kcommand.h>


struct kcommand_s {
	char *name;
	char *help;
	faux_list_t *params;
};

// Simple attributes

// Name
KGET_STR(command, name);
KSET_STR_ONCE(command, name);

// Help
KGET_STR(command, help);
KSET_STR(command, help);

// PARAM list
KCMP_NESTED_BY_KEY(command, param, name);
KADD_NESTED(command, param);
KFIND_NESTED(command, param);


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

	command->params = faux_list_new(FAUX_LIST_UNSORTED, FAUX_LIST_UNIQUE,
		NULL, kcommand_param_kcompare,
		(void (*)(void *))kcommand_free);
	assert(command->params);

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
	bool_t retval = BOOL_TRUE;

	// Name [mandatory]
	if (faux_str_is_empty(info->name)) {
		if (error)
			*error = KCOMMAND_ERROR_ATTR_NAME;
		retval = BOOL_FALSE;
	} else {
		if (!kcommand_set_name(command, info->name)) {
			if (error)
				*error = KCOMMAND_ERROR_ATTR_NAME;
			retval = BOOL_FALSE;
		}
	}

	// Help
	if (!faux_str_is_empty(info->name)) {
		if (!kcommand_set_help(command, info->help)) {
			if (error)
				*error = KCOMMAND_ERROR_ATTR_HELP;
			retval = BOOL_FALSE;
		}
	}

	return retval;
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

/*
	// ACTION list
	if (icommand->actions) {
		iaction_t **p_iaction = NULL;
		for (p_iaction = *icommand->actions; *p_iaction; p_iaction++) {
			kaction_t *kaction = NULL;
			iaction_t *iaction = *p_iaction;
iaction = iaction;
printf("action\n");
//			kaction = kaction_from_iaction(iaction, error_stack);
//			if (!kaction) {
//				retval = BOOL_FALSE;
//				continue;
//			}
kaction = kaction;
		}
	}
*/
	return retval;
}


kcommand_t *kcommand_from_icommand(icommand_t *icommand, faux_error_t *error_stack)
{
	kcommand_t *kcommand = NULL;
	kcommand_error_e kcommand_error = KCOMMAND_ERROR_OK;

	kcommand = kcommand_new(icommand, &kcommand_error);
	if (!kcommand) {
		char *msg = NULL;
		msg = faux_str_sprintf("COMMAND \"%s\": %s",
			icommand->name ? icommand->name : "(null)",
			kcommand_strerror(kcommand_error));
		faux_error_add(error_stack, msg);
		faux_str_free(msg);
		return NULL;
	}
	printf("command %s\n", kcommand_name(kcommand));

	// Parse nested elements
	if (!kcommand_nested_from_icommand(kcommand, icommand, error_stack)) {
		char *msg = NULL;
		msg = faux_str_sprintf("COMMAND \"%s\": Illegal nested elements",
			kcommand_name(kcommand));
		faux_error_add(error_stack, msg);
		faux_str_free(msg);
		kcommand_free(kcommand);
		return NULL;
	}

	return kcommand;
}
