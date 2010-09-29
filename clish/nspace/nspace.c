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
	this->proxy_cmd = NULL;
}

/*--------------------------------------------------------- */
static void clish_nspace_fini(clish_nspace_t * this)
{
	/* deallocate the memory for this instance */
	lub_string_free(this->prefix);
	this->prefix = NULL;

	if (this->proxy_cmd) {
		clish_command_delete(this->proxy_cmd);
		this->proxy_cmd = NULL;
	}
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
	char *name = NULL;

	if (this->proxy_cmd) {
		clish_command_delete(this->proxy_cmd);
		this->proxy_cmd = NULL;
	}

	assert(prefix);
	if (!ref) {
		assert(this->prefix_cmd);
		this->proxy_cmd = clish_command_new_link(prefix, this->prefix_cmd);
		return this->proxy_cmd;
	}

	lub_string_catn(&name, prefix, strlen(prefix));
	lub_string_catn(&name, " ", 1);
	lub_string_catn(&name, clish_command__get_name(ref),
			strlen(clish_command__get_name(ref)));

	this->proxy_cmd = clish_command_new_link(name, ref);
	free(name);

	return this->proxy_cmd;
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
	const char *line)
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
	if (res || (-1 == pmatch[0].rm_so))
		return NULL;
	in_line = line + pmatch[0].rm_eo;

	/* If entered line contain only the prefix */
	if (in_line[0] == '\0')
		return NULL;

	/* If prefix is not followed by space */
	if (in_line[0] != ' ')
		return NULL;

	return (in_line + 1);
}

/*--------------------------------------------------------- */
clish_command_t *clish_nspace_find_command(clish_nspace_t * this, const char *name)
{
	clish_command_t *cmd = NULL, *cmd_link = NULL;
	clish_view_t *view = clish_nspace__get_view(this);
	const char *prefix = clish_nspace__get_prefix(this);
	const char *in_line;

	if (!prefix)
		return clish_view_find_command(view, name, this->inherit);

	if (!(in_line = clish_nspace_after_prefix(prefix, name)))
		return NULL;

	cmd = clish_view_find_command(view, in_line, this->inherit);

	return clish_nspace_find_create_command(this, prefix, cmd);
}

/*--------------------------------------------------------- */
const clish_command_t *clish_nspace_find_next_completion(clish_nspace_t * this,
	const char *iter_cmd, const char *line, clish_nspace_visibility_t field)
{
	const clish_command_t *cmd = NULL, *cmd_link = NULL;
	clish_view_t *view = clish_nspace__get_view(this);
	const char *prefix = clish_nspace__get_prefix(this);
	const char *in_iter = "";
	const char *in_line;

	if (!prefix)
		return clish_view_find_next_completion(view, iter_cmd, line, field, this->inherit);

	if (!(in_line = clish_nspace_after_prefix(prefix, line)))
		return NULL;

	if (iter_cmd &&
	    (lub_string_nocasestr(iter_cmd, prefix) == iter_cmd) &&
	    (lub_string_nocasecmp(iter_cmd, prefix)))
		in_iter = iter_cmd + strlen(prefix) + 1;
	cmd = clish_view_find_next_completion(view, in_iter, in_line, field, this->inherit);
	if (!cmd)
		return NULL;

	return clish_nspace_find_create_command(this, prefix, cmd);
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
