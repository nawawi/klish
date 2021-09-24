/*
 *
 */

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <faux/str.h>
#include <faux/list.h>
#include <klish/kcontext.h>
#include <klish/kentry.h>


int klish_ptype_COMMAND(kcontext_t *context)
{
	kparg_t *parg = NULL;
	const kentry_t *entry = NULL;
	const char *value = NULL;
	const char *command_name = NULL;

	parg = kcontext_candidate_parg(context);
	entry = kparg_entry(parg);
	value = kparg_value(parg);

	command_name = kentry_value(entry);
	if (!command_name)
		command_name = kentry_name(entry);
	if (!command_name)
		return -1;

	return faux_str_casecmp(value, command_name);
}


int klish_ptype_COMMAND_CASE(kcontext_t *context)
{
	kparg_t *parg = NULL;
	const kentry_t *entry = NULL;
	const char *value = NULL;
	const char *command_name = NULL;

	parg = kcontext_candidate_parg(context);
	entry = kparg_entry(parg);
	value = kparg_value(parg);

	command_name = kentry_value(entry);
	if (!command_name)
		command_name = kentry_name(entry);
	if (!command_name)
		return -1;

	return strcmp(value, command_name);
}
