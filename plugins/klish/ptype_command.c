/*
 * Implementation of COMMAND and COMMAND_CASE PTYPEs.
 *
 * This PTYPE compares user typed value with the entry's "value" field if defined
 * or entry's name.
 *
 * The "value" field supports short variants of commands. That means the user
 * can specify something like this 'value="env|ironment"' where "environment"
 * is a full command name and the substring before the "|" is a minimal prefix
 * enough for PTYPE to satisfy comparison and return success as return code.
 * I.e. user can type "env", "envi", "envir", ..., "environment" to execute
 * this command. But the strings "e" or "en" is not a case because their length
 * is less than minimal prefix. Note the short commands are available only for
 * the "value" attribute but not for the "name". Because name is a component of
 * the "path" that allows to address scheme elements.
 *
 * The COMMAND PTYPE uses internal user data attached to the kentry_t structure
 * to store pre-parsed command name with length information. It's necessary to
 * don't parse "value" and "name" fields each time. The PTYPE uses lazy method
 * i.e. parse data on first request and then store results within internal user
 * data.
 */

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <syslog.h>

#include <faux/str.h>
#include <faux/list.h>
#include <faux/conv.h>
#include <faux/argv.h>
#include <klish/kcontext.h>
#include <klish/kentry.h>


// User data structure to pre-parse config setting.
typedef struct {
	char *cmd; // command (not null-terminated) with "len" length
	size_t len; // length of command string
	size_t min_len; // minimal length to satisfy string comparison
} klish_ptype_COMMAND_t;


static void klish_ptype_COMMAND_free(void *data)
{
	klish_ptype_COMMAND_t *udata = (klish_ptype_COMMAND_t *)data;

	faux_str_free(udata->cmd);
	faux_free(udata);
}


klish_ptype_COMMAND_t *klish_ptype_COMMAND_init(kentry_t *entry)
{
	klish_ptype_COMMAND_t *udata = NULL;
	const char *cmd = NULL;
	char *dupcmd = NULL;
	size_t len = 0;
	size_t min_len = 0;
	char *delim = NULL;

	cmd = kentry_value(entry);
	if (cmd) {
		delim = strchr(cmd, '|');
		if (delim) {
			min_len = delim - cmd;
			len = strlen(cmd);
			dupcmd = faux_malloc(len);
			assert(dupcmd);
			if (!dupcmd)
				return NULL;
			memcpy(dupcmd, cmd, min_len);
			memcpy(dupcmd + min_len, delim + 1, len - min_len);
			len--; // exclude trailing '\0'
		} else {
			dupcmd = faux_str_dup(cmd);
			len = strlen(cmd);
			min_len = len;
		}
	} else {
		cmd = kentry_name(entry);
		if (!cmd)
			return NULL;
		dupcmd = faux_str_dup(cmd);
		len = strlen(cmd);
		min_len = len;
	}

	udata = faux_malloc(sizeof(*udata));
	assert(udata);
	if (!udata) {
		faux_free(dupcmd);
		return NULL;
	}

	udata->cmd = dupcmd;
	udata->len = len;
	udata->min_len = min_len;

	kentry_set_udata(entry, udata, klish_ptype_COMMAND_free);

	return udata;
}


/** @brief PTYPE: Consider ENTRY's name (or "value" field) as a command
 */
int klish_ptype_COMMAND(kcontext_t *context)
{
	kentry_t *entry = NULL;
	const char *value = NULL;
	klish_ptype_COMMAND_t *udata = NULL;
	size_t len = 0;

	entry = kcontext_candidate_entry(context);
	value = kcontext_candidate_value(context);

	udata = (klish_ptype_COMMAND_t *)kentry_udata(entry);
	if (!udata) {
		udata = klish_ptype_COMMAND_init(entry);
		if (!udata)
			return -1;
	}

	len = strlen(value);
	if (len < udata->min_len || len > udata->len)
		return -1;

	if (faux_str_casecmpn(value, udata->cmd, len) != 0)
		return -1;

	kcontext_fwrite(context, stdout, udata->cmd, udata->len);

	return 0;
}


/** @brief COMPLETION: Consider ENTRY's name (or "value" field) as a command
 *
 * This completion function has main ENTRY that is a child of COMMAND ptype
 * ENTRY. The PTYPE entry has specific ENTRY (with name and possible value)
 * as a parent.
 *
 * command (COMMON ENTRY) with name or value
 *     ptype (PTYPE ENTRY)
 *         completion (COMPLETION ENTRY) - start point
 */
int klish_completion_COMMAND(kcontext_t *context)
{
	kentry_t *entry = NULL;
	klish_ptype_COMMAND_t *udata = NULL;

	entry = kcontext_candidate_entry(context);
	udata = (klish_ptype_COMMAND_t *)kentry_udata(entry);
	if (!udata) {
		udata = klish_ptype_COMMAND_init(entry);
		if (!udata)
			return -1;
	}

	kcontext_fwrite(context, stdout, udata->cmd, udata->len);

	return 0;
}


/** @brief HELP: Consider ENTRY's name (or "value" field) as a command
 *
 * This help function has main ENTRY that is a child of COMMAND ptype
 * ENTRY. The PTYPE entry has specific ENTRY (with name and possible value)
 * as a parent.
 *
 * command (COMMON ENTRY) with name or value
 *     ptype (PTYPE ENTRY)
 *         help (HELP ENTRY) - start point
 */
int klish_help_COMMAND(kcontext_t *context)
{
	kentry_t *entry = NULL;
	const char *help_text = NULL;
	klish_ptype_COMMAND_t *udata = NULL;

	entry = kcontext_candidate_entry(context);
	udata = (klish_ptype_COMMAND_t *)kentry_udata(entry);
	if (!udata) {
		udata = klish_ptype_COMMAND_init(entry);
		if (!udata)
			return -1;
	}

	help_text = kentry_help(entry);
	if (!help_text)
		help_text = kentry_value(entry);
	if (!help_text)
		help_text = kentry_name(entry);
	assert(help_text);

	kcontext_fwrite(context, stdout, udata->cmd, udata->len);
	kcontext_printf(context, "\n%s\n", help_text);

	return 0;
}


/** @brief PTYPE: ENTRY's name (or "value" field) as a case sensitive command
 */
int klish_ptype_COMMAND_CASE(kcontext_t *context)
{
	kentry_t *entry = NULL;
	const char *value = NULL;
	klish_ptype_COMMAND_t *udata = NULL;
	size_t len = 0;

	entry = kcontext_candidate_entry(context);
	value = kcontext_candidate_value(context);

	udata = (klish_ptype_COMMAND_t *)kentry_udata(entry);
	if (!udata) {
		udata = klish_ptype_COMMAND_init(entry);
		if (!udata)
			return -1;
	}

	len = strlen(value);
	if (len < udata->min_len || len > udata->len)
		return -1;

	if (faux_str_cmpn(value, udata->cmd, len) != 0)
		return -1;

	kcontext_fwrite(context, stdout, udata->cmd, udata->len);

	return 0;
}
