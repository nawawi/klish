#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <faux/conv.h>
#include <faux/list.h>
#include <faux/error.h>
#include <klish/khelper.h>
#include <klish/kaction.h>

#define TAG "ACTION"

bool_t kaction_parse(kaction_t *action, const iaction_t *info, faux_error_t *error)
{
	bool_t retcode = BOOL_TRUE;

	// Sym
	if (!faux_str_is_empty(info->sym)) {
		if (!kaction_set_sym_ref(action, info->sym)) {
			faux_error_add(error, TAG": Illegal 'sym' attribute");
			retcode = BOOL_FALSE;
		}
	}

	// Lock
	if (!faux_str_is_empty(info->lock)) {
		if (!kaction_set_lock(action, info->lock)) {
			faux_error_add(error, TAG": Illegal 'lock' attribute");
			retcode = BOOL_FALSE;
		}
	}

	// Interrupt
	if (!faux_str_is_empty(info->interrupt)) {
		bool_t b = BOOL_FALSE;
		if (!faux_conv_str2bool(info->interrupt, &b) ||
			!kaction_set_interrupt(action, b)) {
			faux_error_add(error, TAG": Illegal 'interrupt' attribute");
			retcode = BOOL_FALSE;
		}
	}

	// Interactive
	if (!faux_str_is_empty(info->interactive)) {
		bool_t b = BOOL_FALSE;
		if (!faux_conv_str2bool(info->interactive, &b) ||
			!kaction_set_interactive(action, b)) {
			faux_error_add(error, TAG": Illegal 'interactive' attribute");
			retcode = BOOL_FALSE;
		}
	}

	// Exec_on
	if (!faux_str_is_empty(info->exec_on)) {
		kaction_cond_e c = KACTION_COND_NONE;
		if (faux_str_casecmp(info->exec_on, "fail"))
			c = KACTION_COND_FAIL;
		else if (faux_str_casecmp(info->exec_on, "success"))
			c = KACTION_COND_SUCCESS;
		else if (faux_str_casecmp(info->exec_on, "always"))
			c = KACTION_COND_ALWAYS;
		if ((KACTION_COND_NONE == c) || !kaction_set_exec_on(action, c)) {
			faux_error_add(error, TAG": Illegal 'exec_on' attribute");
			retcode = BOOL_FALSE;
		}
	}

	// Update_retcode
	if (!faux_str_is_empty(info->update_retcode)) {
		bool_t b = BOOL_FALSE;
		if (!faux_conv_str2bool(info->update_retcode, &b) ||
			!kaction_set_update_retcode(action, b)) {
			faux_error_add(error, TAG": Illegal 'update_retcode' attribute");
			retcode = BOOL_FALSE;
		}
	}

	// Script
	if (!faux_str_is_empty(info->script)) {
		if (!kaction_set_script(action, info->script)) {
			faux_error_add(error, TAG": Illegal 'script' attribute");
			retcode = BOOL_FALSE;
		}
	}

	return retcode;
}


kaction_t *kaction_from_iaction(iaction_t *iaction, faux_error_t *error)
{
	kaction_t *kaction = NULL;

	kaction = kaction_new();
	if (!kaction) {
		faux_error_add(error, TAG": Can't create object");
		return NULL;
	}
	if (!kaction_parse(kaction, iaction, error)) {
		kaction_free(kaction);
		return NULL;
	}

	return kaction;
}
