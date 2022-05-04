#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <faux/conv.h>
#include <faux/list.h>
#include <klish/khelper.h>
#include <klish/kaction.h>
#include <klish/ksym.h>
#include <klish/kplugin.h>


struct kaction_s {
	char *sym_ref; // Text reference to symbol
	ksym_t *sym; // Symbol itself
	kplugin_t *plugin; // Source of symbol
	char *lock; // Named lock
	bool_t interrupt;
	bool_t interactive;
	kaction_cond_e exec_on;
	bool_t update_retcode;
	tri_t permanent;
	tri_t sync;
	char *script;
};


// Simple methods

// Sym reference (must be resolved later)
KGET_STR(action, sym_ref);
KSET_STR_ONCE(action, sym_ref);

// Lock
KGET_STR(action, lock);
KSET_STR(action, lock);

// Interrupt
KGET_BOOL(action, interrupt);
KSET_BOOL(action, interrupt);

// Interactive
KGET_BOOL(action, interactive);
KSET_BOOL(action, interactive);

// Exec_on
KGET(action, kaction_cond_e, exec_on);
KSET(action, kaction_cond_e, exec_on);

// Update_retcode
KGET_BOOL(action, update_retcode);
KSET_BOOL(action, update_retcode);

// Permanent
KGET(action, tri_t, permanent);
KSET(action, tri_t, permanent);

// Sync
KGET(action, tri_t, sync);
KSET(action, tri_t, sync);

// Script
KGET_STR(action, script);
KSET_STR(action, script);

// Symbol
KGET(action, ksym_t *, sym);
KSET(action, ksym_t *, sym);

// Plugin. Source of sym
KGET(action, kplugin_t *, plugin);
KSET(action, kplugin_t *, plugin);


kaction_t *kaction_new(void)
{
	kaction_t *action = NULL;

	action = faux_zmalloc(sizeof(*action));
	assert(action);
	if (!action)
		return NULL;

	// Initialize
	action->sym_ref = NULL;
	action->lock = NULL;
	action->interrupt = BOOL_FALSE;
	action->interactive = BOOL_FALSE;
	action->exec_on = KACTION_COND_SUCCESS;
	action->update_retcode = BOOL_TRUE;
	action->script = NULL;
	action->sym = NULL;
	action->plugin = NULL;

	return action;
}


void kaction_free(kaction_t *action)
{
	if (!action)
		return;

	faux_str_free(action->sym_ref);
	faux_str_free(action->lock);
	faux_str_free(action->script);

	faux_free(action);
}


bool_t kaction_meet_exec_conditions(const kaction_t *action, int current_retcode)
{
	bool_t r = BOOL_FALSE; // Default is pessimistic

	assert(action);
	if (!action)
		return BOOL_FALSE;

	switch (kaction_exec_on(action)) {
	case KACTION_COND_ALWAYS:
		r = BOOL_TRUE;
		break;
	case KACTION_COND_SUCCESS:
		if (0 == current_retcode)
			r = BOOL_TRUE;
		break;
	case KACTION_COND_FAIL:
		if (current_retcode != 0)
			r = BOOL_TRUE;
		break;
	default:
		r = BOOL_FALSE; // NEVER or NONE
	}

	return r;
}


bool_t kaction_is_permanent(const kaction_t *action)
{
	ksym_t *sym = NULL;
	tri_t val = TRI_UNDEFINED;

	assert(action);
	if (!action)
		return BOOL_FALSE;

	sym = kaction_sym(action);
	if (!sym)
		return BOOL_FALSE;

	val = ksym_permanent(sym);
	if (TRI_UNDEFINED == val)
		val = kaction_permanent(action);

	if (TRI_TRUE == val)
		return BOOL_TRUE;

	return BOOL_FALSE; // Default if not set
}


bool_t kaction_is_sync(const kaction_t *action)
{
	ksym_t *sym = NULL;
	tri_t val = TRI_UNDEFINED;

	assert(action);
	if (!action)
		return BOOL_FALSE;

	sym = kaction_sym(action);
	if (!sym)
		return BOOL_FALSE;

	val = ksym_sync(sym);
	if (TRI_UNDEFINED == val)
		val = kaction_sync(action);

	if (TRI_TRUE == val)
		return BOOL_TRUE;

	return BOOL_FALSE; // Default if not set
}
