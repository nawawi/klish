#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <faux/list.h>
#include <klish/kaction.h>


struct kaction_s {
	bool_t is_static;
	kaction_error_e error;
	iaction_t info;
	faux_list_t *commands;
};


static kaction_t *kaction_new_internal(iaction_t info, bool_t is_static)
{
	kaction_t *action = NULL;

	action = faux_zmalloc(sizeof(*action));
	assert(action);
	if (!action)
		return NULL;

	// Initialize
	action->is_static = is_static;
	action->error = KACTION_ERROR_OK;
	action->info = info;

	return action;
}


kaction_t *kaction_new(iaction_t info)
{
	return kaction_new_internal(info, BOOL_FALSE);
}


kaction_t *kaction_new_static(iaction_t info)
{
	return kaction_new_internal(info, BOOL_TRUE);
}


void kaction_free(kaction_t *action)
{
	if (!action)
		return;

	if (!action->is_static) {
		faux_str_free(action->info.sym);
	}

	faux_free(action);
}


const char *kaction_sym_str(const kaction_t *action)
{
	assert(action);
	if (!action)
		return NULL;

	return action->info.sym;
}
