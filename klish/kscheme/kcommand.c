#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <faux/list.h>
#include <klish/kparam.h>
#include <klish/kcommand.h>


struct kcommand_s {
	bool_t is_static;
//	kaction_error_e error;
	icommand_t info;
	faux_list_t *params;
};


static int kcommand_param_compare(const void *first, const void *second)
{
	const kparam_t *f = (const kparam_t *)first;
	const kparam_t *s = (const kparam_t *)second;

	return strcmp(kparam_name(f), kparam_name(s));
}


static int kcommand_param_kcompare(const void *key, const void *list_item)
{
	const char *f = (const char *)key;
	const kparam_t *s = (const kparam_t *)list_item;

	return strcmp(f, kparam_name(s));
}


static kcommand_t *kcommand_new_internal(icommand_t info, bool_t is_static)
{
	kcommand_t *command = NULL;

	command = faux_zmalloc(sizeof(*command));
	assert(command);
	if (!command)
		return NULL;

	// Initialize
	command->is_static = is_static;
//	command->error = KACTION_ERROR_OK;
	command->info = info;

	// List of parameters
	command->params = faux_list_new(FAUX_LIST_UNSORTED, FAUX_LIST_UNIQUE,
		kcommand_param_compare, kcommand_param_kcompare,
		(void (*)(void *))kparam_free);
	assert(command->params);
//	if (!command->params) {
//		command->error = KACTION_ERROR_LIST;
//		return NULL;
//	}

	// Field "exec_on"
//	if (faux_str_casecmp(command->info.

	return command;
}


kcommand_t *kcommand_new(icommand_t info)
{
	return kcommand_new_internal(info, BOOL_FALSE);
}


kcommand_t *kcommand_new_static(icommand_t info)
{
	return kcommand_new_internal(info, BOOL_TRUE);
}


void kcommand_free(kcommand_t *command)
{
	if (!command)
		return;

	if (!command->is_static) {
		faux_str_free(command->info.name);
		faux_str_free(command->info.help);
	}
	faux_list_free(command->params);

	faux_free(command);
}


const char *kcommand_name(const kcommand_t *command)
{
	assert(command);
	if (!command)
		return NULL;

	return command->info.name;
}


const char *kcommand_help(const kcommand_t *command)
{
	assert(command);
	if (!command)
		return NULL;

	return command->info.help;
}


bool_t kcommand_add_param(kcommand_t *command, kparam_t *param)
{
	assert(command);
	if (!command)
		return BOOL_FALSE;
	assert(param);
	if (!param)
		return BOOL_FALSE;

	if (!faux_list_add(command->params, param))
		return BOOL_FALSE;

	return BOOL_TRUE;
}
