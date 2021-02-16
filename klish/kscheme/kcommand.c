#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <klish/kcommand.h>


struct kcommand_s {
	bool_t is_static;
	kcommand_info_t info;
};


static kcommand_t *kcommand_new_internal(kcommand_info_t info, bool_t is_static)
{
	kcommand_t *command = NULL;

	command = faux_zmalloc(sizeof(*command));
	assert(command);
	if (!command)
		return NULL;

	// Initialize
	command->is_static = is_static;
	command->info = info;

	return command;
}


kcommand_t *kcommand_new(kcommand_info_t info)
{
	return kcommand_new_internal(info, BOOL_FALSE);
}


kcommand_t *kcommand_new_static(kcommand_info_t info)
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
