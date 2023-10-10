#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <syslog.h>

#include <faux/str.h>
#include <faux/conv.h>
#include <faux/list.h>
#include <faux/error.h>
#include <klish/khelper.h>
#include <klish/kaction.h>
#include <klish/iaction.h>

#define TAG "ACTION"


static const char * const kaction_io_e_str[] = {
	NULL,
	"false",
	"true",
	"tty",
};


static const char *kaction_io_e_enum2str(kaction_io_e io)
{
	if ((KACTION_IO_NONE == io) || (io >= KACTION_IO_MAX))
		return NULL;

	return kaction_io_e_str[io];
}


static kaction_io_e kaction_io_e_str2enum(const char *str)
{
	kaction_io_e io = KACTION_IO_NONE;

	if (!str)
		return KACTION_IO_NONE;

	for (io = (KACTION_IO_NONE + 1); io < KACTION_IO_MAX; io++) {
		if (faux_str_casecmp(str, kaction_io_e_str[io]) == 0)
			break;
	}
	if (io >= KACTION_IO_MAX)
		return KACTION_IO_NONE;

	return io;
}


bool_t iaction_parse(const iaction_t *info, kaction_t *action, faux_error_t *error)
{
	bool_t retcode = BOOL_TRUE;

	if (!info)
		return BOOL_FALSE;
	if (!action)
		return BOOL_FALSE;

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

	// In
	if (!faux_str_is_empty(info->in)) {
		kaction_io_e io = KACTION_IO_NONE;
		if (((io = kaction_io_e_str2enum(info->in)) == KACTION_IO_NONE) ||
			!kaction_set_in(action, io)) {
			faux_error_add(error, TAG": Illegal 'in' attribute");
			retcode = BOOL_FALSE;
		}
	}

	// Out
	if (!faux_str_is_empty(info->out)) {
		kaction_io_e io = KACTION_IO_NONE;
		if (((io = kaction_io_e_str2enum(info->out)) == KACTION_IO_NONE) ||
			!kaction_set_out(action, io)) {
			faux_error_add(error, TAG": Illegal 'out' attribute");
			retcode = BOOL_FALSE;
		}
	}

	// Exec_on
	if (!faux_str_is_empty(info->exec_on)) {
		kaction_cond_e c = KACTION_COND_NONE;
		if (!faux_str_casecmp(info->exec_on, "fail"))
			c = KACTION_COND_FAIL;
		else if (!faux_str_casecmp(info->exec_on, "success"))
			c = KACTION_COND_SUCCESS;
		else if (!faux_str_casecmp(info->exec_on, "always"))
			c = KACTION_COND_ALWAYS;
		else if (!faux_str_casecmp(info->exec_on, "never"))
			c = KACTION_COND_NEVER;
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

	// Permanent
	if (!faux_str_is_empty(info->permanent)) {
		tri_t b = TRI_UNDEFINED;
		if (!faux_conv_str2tri(info->permanent, &b) ||
			!kaction_set_permanent(action, b)) {
			faux_error_add(error, TAG": Illegal 'permanent' attribute");
			retcode = BOOL_FALSE;
		}
	}

	// Sync
	if (!faux_str_is_empty(info->sync)) {
		tri_t b = TRI_UNDEFINED;
		if (!faux_conv_str2tri(info->sync, &b) ||
			!kaction_set_sync(action, b)) {
			faux_error_add(error, TAG": Illegal 'sync' attribute");
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


kaction_t *iaction_load(const iaction_t *iaction, faux_error_t *error)
{
	kaction_t *kaction = NULL;

	kaction = kaction_new();
	if (!kaction) {
		faux_error_add(error, TAG": Can't create object");
		return NULL;
	}
	if (!iaction_parse(iaction, kaction, error)) {
		kaction_free(kaction);
		return NULL;
	}

	return kaction;
}


char *iaction_deploy(const kaction_t *kaction, int level)
{
	char *str = NULL;
	char *tmp = NULL;
	char *exec_on = NULL;

	if (!kaction)
		return NULL;

	tmp = faux_str_sprintf("%*cACTION {\n", level, ' ');
	faux_str_cat(&str, tmp);
	faux_str_free(tmp);

	attr2ctext(&str, "sym", kaction_sym_ref(kaction), level + 1);
	attr2ctext(&str, "lock", kaction_lock(kaction), level + 1);
	attr2ctext(&str, "interrupt", faux_conv_bool2str(kaction_interrupt(kaction)), level + 1);
	attr2ctext(&str, "in", kaction_io_e_enum2str(kaction_in(kaction)), level + 1);
	attr2ctext(&str, "out", kaction_io_e_enum2str(kaction_out(kaction)), level + 1);
	// Exec_on
	switch (kaction_exec_on(kaction)) {
	case KACTION_COND_FAIL:
		exec_on = "fail";
		break;
	case KACTION_COND_SUCCESS:
		exec_on = "success";
		break;
	case KACTION_COND_ALWAYS:
		exec_on = "always";
		break;
	case KACTION_COND_NEVER:
		exec_on = "never";
		break;
	default:
		exec_on = NULL;
	}
	attr2ctext(&str, "exec_on", exec_on, level + 1);
	attr2ctext(&str, "update_retcode", faux_conv_bool2str(kaction_update_retcode(kaction)), level + 1);
	attr2ctext(&str, "permanent", faux_conv_tri2str(kaction_permanent(kaction)), level + 1);
	attr2ctext(&str, "sync", faux_conv_tri2str(kaction_sync(kaction)), level + 1);
	attr2ctext(&str, "script", kaction_script(kaction), level + 1);

	tmp = faux_str_sprintf("%*c},\n\n", level, ' ');
	faux_str_cat(&str, tmp);
	faux_str_free(tmp);

	return str;
}
