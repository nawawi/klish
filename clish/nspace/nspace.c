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

/*---------------------------------------------------------
 * PRIVATE METHODS
 *--------------------------------------------------------- */
static void clish_nspace_init(clish_nspace_t * this, clish_view_t * view)
{

	this->view = view;

	/* set up defaults */
	this->prefix = NULL;
	this->help = BOOL_FALSE;
	this->completion = BOOL_TRUE;
	this->context_help = BOOL_FALSE;
	this->inherit = BOOL_TRUE;
	this->prefix_cmd = NULL;

	/* initialise the tree of commands links for this nspace */
	lub_bintree_init(&this->tree,
		 clish_command_bt_offset(),
		 clish_command_bt_compare, clish_command_bt_getkey);
}

/*--------------------------------------------------------- */
static void clish_nspace_fini(clish_nspace_t * this)
{
	clish_command_t *cmd;

	/* deallocate the memory for this instance */
	lub_string_free(this->prefix);
	this->prefix = NULL;
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
	clish_command_t *tmp = NULL;
	lub_bintree_iterator_t iter;
	const char *str = NULL;

	assert(prefix);
	if (!ref) {
		assert(this->prefix_cmd);
		name = lub_string_dup(prefix);
		ref = this->prefix_cmd;
	} else {
		lub_string_catn(&name, prefix, strlen(prefix));
		lub_string_catn(&name, " ", 1);
		lub_string_catn(&name, clish_command__get_name(ref),
				strlen(clish_command__get_name(ref)));
	}

	/* The command is cached already */
	if (cmd = lub_bintree_find(&this->tree, name)) {
		free(name);
		return cmd;
	}
	cmd = clish_command_new_link(name, ref);
	free(name);
	assert(cmd);

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

/*---------------------------------------------------------
 * PUBLIC META FUNCTIONS
 *--------------------------------------------------------- */
clish_nspace_t *clish_nspace_new(clish_view_t * view)
{
	clish_nspace_t *this = malloc(sizeof(clish_nspace_t));

	if (this) {
		clish_nspace_init(this, view);
	}
	return this;
}

/*---------------------------------------------------------
 * PUBLIC METHODS
 *--------------------------------------------------------- */
void clish_nspace_delete(clish_nspace_t * this)
{
	clish_nspace_fini(this);
	free(this);
}

/*--------------------------------------------------------- */
static const char *clish_nspace_after_prefix(const char *prefix,
	const char *line, char **real_prefix)
{
	const char *in_line = NULL;
	regex_t regexp;
	regmatch_t pmatch[1];
	int res;

	if (!line)
		return NULL;

	/* Compile regular expression */
	regcomp(&regexp, prefix, REG_EXTENDED | REG_ICASE);
	res = regexec(&regexp, line, 1, pmatch, 0);
	regfree(&regexp);
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
	const char *prefix = clish_nspace__get_prefix(this);
	const char *in_line;
	char *real_prefix = NULL;

	if (!prefix)
		return clish_view_find_command(view, name, this->inherit);

	if (!(in_line = clish_nspace_after_prefix(prefix, name, &real_prefix)))
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
	clish_nspace_visibility_t field)
{
	const clish_command_t *cmd = NULL, *retval = NULL;
	clish_view_t *view = clish_nspace__get_view(this);
	const char *prefix = clish_nspace__get_prefix(this);
	const char *in_iter = "";
	const char *in_line;
	char *real_prefix = NULL;

	if (!prefix)
		return clish_view_find_next_completion(view, iter_cmd,
			line, field, this->inherit);

	if (!(in_line = clish_nspace_after_prefix(prefix, line, &real_prefix)))
		return NULL;

	if (in_line[0] != '\0') {
		/* If prefix is followed by space */
		if (in_line[0] == ' ')
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

/*---------------------------------------------------------
 * PUBLIC ATTRIBUTES
 *--------------------------------------------------------- */
clish_view_t *clish_nspace__get_view(const clish_nspace_t * this)
{
	return this->view;
}

/*--------------------------------------------------------- */
void clish_nspace__set_prefix(clish_nspace_t * this, const char *prefix)
{
	assert(NULL == this->prefix);
	this->prefix = lub_string_dup(prefix);
}

/*--------------------------------------------------------- */
const char *clish_nspace__get_prefix(const clish_nspace_t * this)
{
	return this->prefix;
}

/*--------------------------------------------------------- */
void clish_nspace__set_help(clish_nspace_t * this, bool_t help)
{
	this->help = help;
}

/*--------------------------------------------------------- */
bool_t clish_nspace__get_help(const clish_nspace_t * this)
{
	return this->help;
}

/*--------------------------------------------------------- */
void clish_nspace__set_completion(clish_nspace_t * this, bool_t completion)
{
	this->completion = completion;
}

/*--------------------------------------------------------- */
bool_t clish_nspace__get_completion(const clish_nspace_t * this)
{
	return this->completion;
}

/*--------------------------------------------------------- */
void clish_nspace__set_context_help(clish_nspace_t * this, bool_t context_help)
{
	this->context_help = context_help;
}

/*--------------------------------------------------------- */
bool_t clish_nspace__get_context_help(const clish_nspace_t * this)
{
	return this->context_help;
}

/*--------------------------------------------------------- */
void clish_nspace__set_inherit(clish_nspace_t * this, bool_t inherit)
{
	this->inherit = inherit;
}

/*--------------------------------------------------------- */
bool_t clish_nspace__get_inherit(const clish_nspace_t * this)
{
	return this->inherit;
}

/*--------------------------------------------------------- */
bool_t
clish_nspace__get_visibility(const clish_nspace_t * instance,
			     clish_nspace_visibility_t field)
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
	}

	return result;
}

/*--------------------------------------------------------- */
