/*
 * Implementation of standard PTYPEs
 */

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <faux/str.h>
#include <faux/list.h>
#include <faux/conv.h>
#include <faux/argv.h>
#include <klish/kcontext.h>
#include <klish/kentry.h>


/** @brief PTYPE: Consider ENTRY's name (or "value" field) as a command
 */
int klish_ptype_COMMAND(kcontext_t *context)
{
	const kentry_t *entry = NULL;
	const char *value = NULL;
	const char *command_name = NULL;

	entry = kcontext_candidate_entry(context);
	value = kcontext_candidate_value(context);

	command_name = kentry_value(entry);
	if (!command_name)
		command_name = kentry_name(entry);
	if (!command_name)
		return -1;

	return faux_str_casecmp(value, command_name);
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
	const kentry_t *entry = NULL;
	const char *command_name = NULL;

	entry = kcontext_candidate_entry(context);

	command_name = kentry_value(entry);
	if (!command_name)
		command_name = kentry_name(entry);
	if (!command_name)
		return 0;

	printf("%s\n", command_name);

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
	const kentry_t *entry = NULL;
	const char *command_name = NULL;
	const char *help_text = NULL;

	entry = kcontext_candidate_entry(context);

	command_name = kentry_value(entry);
	if (!command_name)
		command_name = kentry_name(entry);
	assert(command_name);

	help_text = kentry_help(entry);
	if (!help_text)
		help_text = kentry_value(entry);
	if (!help_text)
		help_text = kentry_name(entry);
	assert(help_text);

	printf("%s\n%s\n", command_name, help_text);

	return 0;
}


/** @brief PTYPE: ENTRY's name (or "value" field) as a case sensitive command
 */
int klish_ptype_COMMAND_CASE(kcontext_t *context)
{
	const kentry_t *entry = NULL;
	const char *value = NULL;
	const char *command_name = NULL;

	entry = kcontext_candidate_entry(context);
	value = kcontext_candidate_value(context);

	command_name = kentry_value(entry);
	if (!command_name)
		command_name = kentry_name(entry);
	if (!command_name)
		return -1;

	return strcmp(value, command_name);
}


/** @brief PTYPE: Signed int with optional range
 *
 * Use long long int for conversion from text.
 *
 * <ACTION sym="INT">-30 80</ACTION>
 * Means range from "-30" to "80"
 */
int klish_ptype_INT(kcontext_t *context)
{
	const char *script = NULL;
	const char *value_str = NULL;
	long long int value = 0;

	script = kcontext_script(context);
	value_str = kcontext_candidate_value(context);

	if (!faux_conv_atoll(value_str, &value, 0))
		return -1;

	// Range is specified
	if (!faux_str_is_empty(script)) {
		faux_argv_t *argv = faux_argv_new();
		const char *str = NULL;
		faux_argv_parse(argv, script);

		// Min
		str = faux_argv_index(argv, 0);
		if (str) {
			long long int min = 0;
			if (!faux_conv_atoll(str, &min, 0) || (value < min)) {
				faux_argv_free(argv);
				return -1;
			}
		}

		// Max
		str = faux_argv_index(argv, 1);
		if (str) {
			long long int max = 0;
			if (!faux_conv_atoll(str, &max, 0) || (value > max)) {
				faux_argv_free(argv);
				return -1;
			}
		}

		faux_argv_free(argv);
	}

	return 0;
}


/** @brief PTYPE: Unsigned int with optional range
 *
 * Use unsigned long long int for conversion from text.
 *
 * <ACTION sym="UINT">30 80</ACTION>
 * Means range from "30" to "80"
 */
int klish_ptype_UINT(kcontext_t *context)
{
	const char *script = NULL;
	const char *value_str = NULL;
	unsigned long long int value = 0;

	script = kcontext_script(context);
	value_str = kcontext_candidate_value(context);

	if (!faux_conv_atoull(value_str, &value, 0))
		return -1;

	// Range is specified
	if (!faux_str_is_empty(script)) {
		faux_argv_t *argv = faux_argv_new();
		const char *str = NULL;
		faux_argv_parse(argv, script);

		// Min
		str = faux_argv_index(argv, 0);
		if (str) {
			unsigned long long int min = 0;
			if (!faux_conv_atoull(str, &min, 0) || (value < min)) {
				faux_argv_free(argv);
				return -1;
			}
		}

		// Max
		str = faux_argv_index(argv, 1);
		if (str) {
			unsigned long long int max = 0;
			if (!faux_conv_atoull(str, &max, 0) || (value > max)) {
				faux_argv_free(argv);
				return -1;
			}
		}

		faux_argv_free(argv);
	}

	return 0;
}


/** @brief PTYPE: Arbitrary string
 */
int klish_ptype_STRING(kcontext_t *context)
{
	// Really any string is a ... (surprise!) string

	context = context; // Happy compiler

	return 0;
}
