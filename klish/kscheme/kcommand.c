#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <faux/list.h>
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

// Help
KGET_STR(command, help);
KSET_STR(command, help);

// PARAM list
static KCMP_NESTED(command, param, name);
static KCMP_NESTED_BY_KEY(command, param, name);
KADD_NESTED(command, param);
KFIND_NESTED(command, param);
KNESTED_LEN(command, param);
KNESTED_ITER(command, param);
KNESTED_EACH(command, param);

// ACTION list
KADD_NESTED(command, action);
KNESTED_LEN(command, action);
KNESTED_ITER(command, action);
KNESTED_EACH(command, action);


kcommand_t *kcommand_new(const char *name)
{
	kcommand_t *command = NULL;

	if (faux_str_is_empty(name))
		return NULL;

	command = faux_zmalloc(sizeof(*command));
	assert(command);
	if (!command)
		return NULL;

	// Initialize
	command->name = faux_str_dup(name);
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
