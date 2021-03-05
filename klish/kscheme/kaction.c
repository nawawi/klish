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


struct kaction_s {
	char *sym_ref; // Text reference to symbol
	char *lock; // Named lock
	bool_t interrupt;
	bool_t interactive;
	kaction_cond_e exec_on;
	bool_t update_retcode;
	char *script;
	ksym_t *sym; // Symbol
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

// Script
KGET_STR(action, script);
KSET_STR(action, script);

// Symbol
KGET(action, ksym_t *, sym);
KSET(action, ksym_t *, sym);


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
