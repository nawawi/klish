#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <faux/conv.h>
#include <faux/list.h>
#include <klish/khelper.h>
#include <klish/kaction.h>


struct kaction_s {
	char *sym_ref; // Text reference to symbol
	char *lock; // Named lock
	bool_t interrupt;
	bool_t interactive;
	kaction_cond_e exec_on;
	bool_t update_retcode;
	char *script;
	//ksym_t *sym; // Symbol
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


static kaction_t *kaction_new_empty(void)
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

	return action;
}


kaction_t *kaction_new(const iaction_t *info, kaction_error_e *error)
{
	kaction_t *action = NULL;

	action = kaction_new_empty();
	assert(action);
	if (!action) {
		if (error)
			*error = KACTION_ERROR_ALLOC;
		return NULL;
	}

	if (!info)
		return action;

	if (!kaction_parse(action, info, error)) {
		kaction_free(action);
		return NULL;
	}

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


const char *kaction_strerror(kaction_error_e error)
{
	const char *str = NULL;

	switch (error) {
	case KACTION_ERROR_OK:
		str = "Ok";
		break;
	case KACTION_ERROR_INTERNAL:
		str = "Internal error";
		break;
	case KACTION_ERROR_ALLOC:
		str = "Memory allocation error";
		break;
	case KACTION_ERROR_ATTR_SYM:
		str = "Illegal 'sym' attribute";
		break;
	case KACTION_ERROR_ATTR_LOCK:
		str = "Illegal 'lock' attribute";
		break;
	case KACTION_ERROR_ATTR_INTERRUPT:
		str = "Illegal 'interrupt' attribute";
		break;
	case KACTION_ERROR_ATTR_INTERACTIVE:
		str = "Illegal 'interactive' attribute";
		break;
	case KACTION_ERROR_ATTR_UPDATE_RETCODE:
		str = "Illegal 'update_retcode' attribute";
		break;
	case KACTION_ERROR_ATTR_SCRIPT:
		str = "Illegal script";
		break;
	default:
		str = "Unknown error";
		break;
	}

	return str;
}


bool_t kaction_parse(kaction_t *action, const iaction_t *info, kaction_error_e *error)
{
	// Sym
	if (!faux_str_is_empty(info->sym)) {
		if (!kaction_set_sym_ref(action, info->sym)) {
			if (error)
				*error = KACTION_ERROR_ATTR_SYM;
			return BOOL_FALSE;
		}
	}

	// Lock
	if (!faux_str_is_empty(info->lock)) {
		if (!kaction_set_lock(action, info->lock)) {
			if (error)
				*error = KACTION_ERROR_ATTR_LOCK;
			return BOOL_FALSE;
		}
	}

	// Interrupt
	if (!faux_str_is_empty(info->interrupt)) {
		bool_t b = BOOL_FALSE;
		if (!faux_conv_str2bool(info->interrupt, &b) ||
			!kaction_set_interrupt(action, b)) {
			if (error)
				*error = KACTION_ERROR_ATTR_INTERRUPT;
			return BOOL_FALSE;
		}
	}

	// Interactive
	if (!faux_str_is_empty(info->interactive)) {
		bool_t b = BOOL_FALSE;
		if (!faux_conv_str2bool(info->interactive, &b) ||
			!kaction_set_interactive(action, b)) {
			if (error)
				*error = KACTION_ERROR_ATTR_INTERACTIVE;
			return BOOL_FALSE;
		}
	}

	// Exec_on
	if (!faux_str_is_empty(info->exec_on)) {
		kaction_cond_e c = KACTION_COND_SUCCESS;
		if (faux_str_casecmp(info->exec_on, "fail"))
			c = KACTION_COND_FAIL;
		else if (faux_str_casecmp(info->exec_on, "success"))
			c = KACTION_COND_SUCCESS;
		else if (faux_str_casecmp(info->exec_on, "always"))
			c = KACTION_COND_ALWAYS;
		else {
			if (error)
				*error = KACTION_ERROR_ATTR_EXEC_ON;
			return BOOL_FALSE;
		}
		if (!kaction_set_exec_on(action, c)) {
			if (error)
				*error = KACTION_ERROR_ATTR_EXEC_ON;
			return BOOL_FALSE;
		}
	}

	// Update_retcode
	if (!faux_str_is_empty(info->update_retcode)) {
		bool_t b = BOOL_FALSE;
		if (!faux_conv_str2bool(info->update_retcode, &b) ||
			!kaction_set_update_retcode(action, b)) {
			if (error)
				*error = KACTION_ERROR_ATTR_UPDATE_RETCODE;
			return BOOL_FALSE;
		}
	}

	// Script
	if (!faux_str_is_empty(info->script)) {
		if (!kaction_set_script(action, info->script)) {
			if (error)
				*error = KACTION_ERROR_ATTR_SCRIPT;
			return BOOL_FALSE;
		}
	}

	return BOOL_TRUE;
}


kaction_t *kaction_from_iaction(iaction_t *iaction, faux_error_t *error_stack)
{
	kaction_t *kaction = NULL;
	kaction_error_e kaction_error = KACTION_ERROR_OK;

	kaction = kaction_new(iaction, &kaction_error);
	if (!kaction) {
		char *msg = NULL;
		msg = faux_str_sprintf("ACTION : %s",
			kaction_strerror(kaction_error));
		faux_error_add(error_stack, msg);
		faux_str_free(msg);
		return NULL;
	}
	printf("action\n");

	return kaction;
}
