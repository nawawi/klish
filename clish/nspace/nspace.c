/*
 * nspace.c
 *
 * This file provides the implementation of the "nspace" class
 */
#include "private.h"
#include "lub/string.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <regex.h>
#include <ctype.h>

/*--------------------------------------------------------- */
static void clish_nspace_init(clish_nspace_t *this,  const char *view_name)
{
	this->view_name = NULL;
	clish_nspace__set_view_name(this, view_name);

	/* Set up defaults */
	this->view = NULL;
	this->prefix = NULL;
	this->help = BOOL_FALSE;
	this->completion = BOOL_TRUE;
	this->context_help = BOOL_FALSE;
	this->inherit = BOOL_TRUE;
	this->prefix_cmd = NULL;
	this->access = NULL;

	/* initialise the tree of commands links for this nspace */
	lub_bintree_init(&this->tree,
		clish_command_bt_offset(),
		clish_command_bt_compare, clish_command_bt_getkey);
}

/*--------------------------------------------------------- */
static void clish_nspace_fini(clish_nspace_t *this)
{
	clish_command_t *cmd;

	/* deallocate the memory for this instance */
	if (this->prefix) {
		free(this->prefix);
		regfree(&this->prefix_regex);
	}
	/* delete each command link held by this nspace */
	while ((cmd = lub_bintree_findfirst(&this->tree))) {
		/* remove the command from the tree */
		lub_bintree_remove(&this->tree, cmd);
		/* release the instance */
		clish_command_delete(cmd);
	}
	/* Delete prefix pseudo-command */
	if (this->prefix_cmd) {
		clish_command_delete(this->prefix_cmd);
		this->prefix_cmd = NULL;
	}
	lub_string_free(this->access);
	lub_string_free(this->view_name);
}

/*--------------------------------------------------------- */
clish_command_t * clish_nspace_create_prefix_cmd(clish_nspace_t * this,
	const char * name, const char * help)
{
	if (this->prefix_cmd) {
		clish_command_delete(this->prefix_cmd);
		this->prefix_cmd = NULL;
	}

	return (this->prefix_cmd = clish_command_new(name, help));
}

/*--------------------------------------------------------- */
static clish_command_t *clish_nspace_find_create_command(clish_nspace_t * this,
	const char *prefix, const clish_command_t * ref)
{
	clish_command_t *cmd;
	char *name = NULL;
	const char *help = NULL;
	clish_command_t *tmp = NULL;
	const char *str = NULL;

	assert(prefix);
	if (!ref) {
		assert(this->prefix_cmd);
		name = lub_string_dup(prefix);
		ref = this->prefix_cmd;
		help = clish_command__get_text(this->prefix_cmd);
	} else {
		lub_string_catn(&name, prefix, strlen(prefix));
		lub_string_catn(&name, " ", 1);
		lub_string_catn(&name, clish_command__get_name(ref),
				strlen(clish_command__get_name(ref)));
		help = clish_command__get_text(ref);
	}

	/* The command is cached already */
	if ((cmd = lub_bintree_find(&this->tree, name))) {
		free(name);
		return cmd;
	}
	cmd = clish_command_new_link(name, help, ref);
	free(name);
	assert(cmd);
	/* The command was created dynamically */
	clish_command__set_dynamic(cmd, BOOL_TRUE);

	/* Delete proxy commands with another prefixes */
	tmp = lub_bintree_findfirst(&this->tree);
	if (tmp)
		str = clish_command__get_name(tmp);
	if (str && (lub_string_nocasestr(str, prefix) != str)) {
		do {
			lub_bintree_remove(&this->tree, tmp);
			clish_command_delete(tmp);
		} while ((tmp = lub_bintree_findfirst(&this->tree)));
	}

	/* Insert command link into the tree */
	if (-1 == lub_bintree_insert(&this->tree, cmd)) {
		clish_command_delete(cmd);
		cmd = NULL;
	}

	return cmd;
}

/*--------------------------------------------------------- */
clish_nspace_t *clish_nspace_new(const char *view_name)
{
	clish_nspace_t *this = malloc(sizeof(clish_nspace_t));

	if (this)
		clish_nspace_init(this, view_name);
	return this;
}

/*--------------------------------------------------------- */
void clish_nspace_delete(void *data)
{
	clish_nspace_t *this = (clish_nspace_t *)data;
	clish_nspace_fini(this);
	free(this);
}

/*--------------------------------------------------------- */
static const char *clish_nspace_after_prefix(const regex_t *prefix_regex,
	const char *line, char **real_prefix)
{
	const char *in_line = NULL;
	regmatch_t pmatch[1] = {};
	int res;

	if (!line)
		return NULL;

	/* Compile regular expression */
	res = regexec(prefix_regex, line, 1, pmatch, 0);
	if (res || (0 != pmatch[0].rm_so))
		return NULL;
	/* Empty match */
	if (0 == pmatch[0].rm_eo)
		return NULL;
	in_line = line + pmatch[0].rm_eo;

	lub_string_catn(real_prefix, line, pmatch[0].rm_eo);

	return in_line;
}

/*--------------------------------------------------------- */
clish_command_t *clish_nspace_find_command(clish_nspace_t * this, const char *name)
{
	clish_command_t *cmd = NULL, *retval = NULL;
	clish_view_t *view = clish_nspace__get_view(this);
	const char *in_line;
	char *real_prefix = NULL;

	if (!clish_nspace__get_prefix(this))
		return clish_view_find_command(view, name, this->inherit);

	if (!(in_line = clish_nspace_after_prefix(
		clish_nspace__get_prefix_regex(this), name, &real_prefix)))
		return NULL;

	/* If prefix is followed by space */
	if (in_line[0] == ' ')
		in_line++;

	if (in_line[0] != '\0') {
		cmd = clish_view_find_command(view, in_line, this->inherit);
		if (!cmd) {
			lub_string_free(real_prefix);
			return NULL;
		}
	}

	retval = clish_nspace_find_create_command(this, real_prefix, cmd);
	lub_string_free(real_prefix);

	return retval;
}

/*--------------------------------------------------------- */
const clish_command_t *clish_nspace_find_next_completion(clish_nspace_t * this,
	const char *iter_cmd, const char *line,
	clish_nspace_visibility_e field)
{
	const clish_command_t *cmd = NULL, *retval = NULL;
	clish_view_t *view = clish_nspace__get_view(this);
	const char *in_iter = "";
	const char *in_line;
	char *real_prefix = NULL;

	if (!clish_nspace__get_prefix(this))
		return clish_view_find_next_completion(view, iter_cmd,
			line, field, this->inherit);

	if (!(in_line = clish_nspace_after_prefix(
		clish_nspace__get_prefix_regex(this), line, &real_prefix)))
		return NULL;

	if (in_line[0] != '\0') {
		/* If prefix is followed by space */
		if (!isspace(in_line[0])) {
			lub_string_free(real_prefix);
			return NULL;
		}
		in_line++;
		if (iter_cmd &&
			(lub_string_nocasestr(iter_cmd, real_prefix) == iter_cmd) &&
			(lub_string_nocasecmp(iter_cmd, real_prefix)))
			in_iter = iter_cmd + strlen(real_prefix) + 1;
		cmd = clish_view_find_next_completion(view,
			in_iter, in_line, field, this->inherit);
		if (!cmd) {
			lub_string_free(real_prefix);
			return NULL;
		}
	}

	/* If prefix was already returned. */
	if (!cmd && iter_cmd && !lub_string_nocasecmp(iter_cmd, real_prefix)) {
		lub_string_free(real_prefix);
		return NULL;
	}

	retval = clish_nspace_find_create_command(this, real_prefix, cmd);
	lub_string_free(real_prefix);

	if (retval && iter_cmd &&
		lub_string_nocasecmp(iter_cmd, clish_command__get_name(retval)) > 0)
		return NULL;

	return retval;
}

/*--------------------------------------------------------- */
void clish_nspace_clean_proxy(clish_nspace_t * this)
{
	clish_command_t *cmd = NULL;

	/* Recursive proxy clean */
	clish_view_clean_proxy(this->view);
	/* Delete each command proxy held by this nspace */
	while ((cmd = lub_bintree_findfirst(&this->tree))) {
		/* remove the command from the tree */
		lub_bintree_remove(&this->tree, cmd);
		/* release the instance */
		clish_command_delete(cmd);
	}
}

CLISH_SET(nspace, bool_t, help);
CLISH_GET(nspace, bool_t, help);
CLISH_SET(nspace, bool_t, completion);
CLISH_GET(nspace, bool_t, completion);
CLISH_SET(nspace, bool_t, context_help);
CLISH_GET(nspace, bool_t, context_help);
CLISH_SET(nspace, bool_t, inherit);
CLISH_GET(nspace, bool_t, inherit);
CLISH_SET(nspace, clish_view_t *, view);
CLISH_GET(nspace, clish_view_t *, view);
CLISH_SET_STR(nspace, view_name);
CLISH_GET_STR(nspace, view_name);
CLISH_SET_STR(nspace, access);
CLISH_GET_STR(nspace, access);
CLISH_GET_STR(nspace, prefix);

/*--------------------------------------------------------- */
_CLISH_SET_STR_ONCE(nspace, prefix)
{
	int res = 0;

	assert(inst);
	assert(!inst->prefix);
	res = regcomp(&inst->prefix_regex, val, REG_EXTENDED | REG_ICASE);
	assert(!res);
	inst->prefix = lub_string_dup(val);
}

/*--------------------------------------------------------- */
_CLISH_GET(nspace, const regex_t *, prefix_regex)
{
	assert(inst);
	if (!inst->prefix)
		return NULL;
	return &inst->prefix_regex;
}

/*--------------------------------------------------------- */
bool_t clish_nspace__get_visibility(const clish_nspace_t * instance,
	clish_nspace_visibility_e field)
{
	bool_t result = BOOL_FALSE;

	switch (field) {
	case CLISH_NSPACE_HELP:
		result = clish_nspace__get_help(instance);
		break;
	case CLISH_NSPACE_COMPLETION:
		result = clish_nspace__get_completion(instance);
		break;
	case CLISH_NSPACE_CHELP:
		result = clish_nspace__get_context_help(instance);
		break;
	default:
		break;
	}

	return result;
}
