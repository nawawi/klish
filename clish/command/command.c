/*
  * command.c
  *
  * This file provides the implementation of a command definition  
  */
#include "private.h"
#include "clish/variable.h"
#include "lub/bintree.h"
#include "lub/string.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*---------------------------------------------------------
 * PRIVATE METHODS
 *--------------------------------------------------------- */
static void
clish_command_init(clish_command_t * this, const char *name, const char *text)
{
	/* initialise the node part */
	this->name = lub_string_dup(name);
	this->text = lub_string_dup(text);

	/* Be a good binary tree citizen */
	lub_bintree_node_init(&this->bt_node);

	/* set up defaults */
	this->link = BOOL_FALSE;
	this->paramv = clish_paramv_new();
	this->viewid = NULL;
	this->view = NULL;
	this->action = NULL;
	this->detail = NULL;
	this->builtin = NULL;
	this->escape_chars = NULL;
	this->args = NULL;
	this->pview = NULL;
	this->cfg_op = CLISH_CONFIG_NONE;
	this->priority = 0x7f00; /* medium priority by default */
	this->pattern = NULL;
	this->file = NULL;
	this->splitter = BOOL_TRUE;
}

/*--------------------------------------------------------- */
static void clish_command_fini(clish_command_t * this)
{
	unsigned i;

	lub_string_free(this->name);
	this->name = NULL;

	/* Link need not full cleanup */
	if (this->link)
		return;

	/* finalize each of the parameter instances */
	clish_paramv_delete(this->paramv);

	lub_string_free(this->viewid);
	this->viewid = NULL;
	lub_string_free(this->action);
	this->action = NULL;
	lub_string_free(this->text);
	this->text = NULL;
	lub_string_free(this->detail);
	this->detail = NULL;
	lub_string_free(this->builtin);
	this->builtin = NULL;
	lub_string_free(this->escape_chars);
	this->escape_chars = NULL;

	if (NULL != this->args) {
		clish_param_delete(this->args);
		this->args = NULL;
	}

	this->pview = NULL;
	lub_string_free(this->pattern);
	this->pattern = NULL;
	lub_string_free(this->file);
	this->file = NULL;
}

/*---------------------------------------------------------
 * PUBLIC META FUNCTIONS
 *--------------------------------------------------------- */
size_t clish_command_bt_offset(void)
{
	return offsetof(clish_command_t, bt_node);
}

/*--------------------------------------------------------- */
int clish_command_bt_compare(const void *clientnode, const void *clientkey)
{
	const clish_command_t *this = clientnode;
	const char *key = clientkey;

	return lub_string_nocasecmp(this->name, key);
}

/*--------------------------------------------------------- */
void clish_command_bt_getkey(const void *clientnode, lub_bintree_key_t * key)
{
	const clish_command_t *this = clientnode;

	/* fill out the opaque key */
	strcpy((char *)key, this->name);
}

/*--------------------------------------------------------- */
clish_command_t *clish_command_new(const char *name, const char *text)
{
	clish_command_t *this = malloc(sizeof(clish_command_t));

	if (this) {
		clish_command_init(this, name, text);
	}
	return this;
}

/*--------------------------------------------------------- */
clish_command_t *clish_command_new_link(const char *name,
					const clish_command_t * ref)
{
	if (!ref)
		return NULL;

	clish_command_t *this = malloc(sizeof(clish_command_t));
	assert(this);

	/* Copy all fields to the new command-link */
	*this = *ref;
	/* Initialise the name (other than original name) */
	this->name = lub_string_dup(name);
	/* Be a good binary tree citizen */
	lub_bintree_node_init(&this->bt_node);
	/* It a link to command so set the link flag */
	this->link = BOOL_TRUE;

	return this;
}

/*---------------------------------------------------------
 * PUBLIC METHODS
 *--------------------------------------------------------- */
void clish_command_delete(clish_command_t * this)
{
	clish_command_fini(this);
	free(this);
}

/*--------------------------------------------------------- */
void clish_command_insert_param(clish_command_t * this, clish_param_t * param)
{
	clish_paramv_insert(this->paramv, param);
}

/*--------------------------------------------------------- */
void clish_command_help(const clish_command_t * this, const char *line)
{
	const char *name = clish_command__get_name(this);
	unsigned index = lub_argv_wordcount(line);
	const clish_param_t *param;
	unsigned idx = lub_argv_wordcount(name);
	lub_argv_t *argv;
	clish_pargv_t *last, *pargv;
	unsigned i;
	unsigned cnt = 0;
	unsigned longest = 0;

	if (0 == index)
		return;

	/* Line has no any parameters */
	if (strlen(line) <= strlen(name)) {
		printf("%s  %s\n", name,
			clish_command__get_text(this));
		return;
	}

	if (line[strlen(line) - 1] != ' ')
		index--;

	argv = lub_argv_new(line, 0);

	/* get the parameter definition */
	last = clish_pargv_create();
	pargv = clish_pargv_create();
	clish_pargv_parse(pargv, this, this->paramv,
		argv, &idx, last, index);
	clish_pargv_delete(pargv);
	cnt = clish_pargv__get_count(last);

	/* Calculate the longest name */
	for (i = 0; i < cnt; i++) {
		const clish_param_t *param;
		const char *name;
		unsigned clen = 0;

		param = clish_pargv__get_param(last, i);
		if (CLISH_PARAM_SUBCOMMAND == clish_param__get_mode(param))
			name = clish_param__get_name(param);
		else
			name = clish_ptype__get_text(clish_param__get_ptype(param));
		if (name)
			clen = strlen(name);
		if (clen > longest)
			longest = clen;
	}
	for (i = 0; i < cnt; i++)
		clish_param_help(clish_pargv__get_param(last, i), longest + 1);
	clish_pargv_delete(last);

	lub_argv_delete(argv);
}

/*--------------------------------------------------------- */
clish_command_t *clish_command_choose_longest(clish_command_t * cmd1,
					      clish_command_t * cmd2)
{
	unsigned len1 = (cmd1 ? strlen(clish_command__get_name(cmd1)) : 0);
	unsigned len2 = (cmd2 ? strlen(clish_command__get_name(cmd2)) : 0);

	if (len2 < len1) {
		return cmd1;
	} else if (len1 < len2) {
		return cmd2;
	} else {
		/* let local view override */
		return cmd1;
	}
}

/*--------------------------------------------------------- */
int
clish_command_diff(const clish_command_t * cmd1, const clish_command_t * cmd2)
{
	if (NULL == cmd1) {
		if (NULL != cmd2) {
			return 1;
		} else {
			return 0;
		}
	}
	if (NULL == cmd2) {
		return -1;
	}
	return lub_string_nocasecmp(clish_command__get_name(cmd1),
				    clish_command__get_name(cmd2));
}

/*---------------------------------------------------------
 * PUBLIC ATTRIBUTES
 *--------------------------------------------------------- */
const char *clish_command__get_name(const clish_command_t * this)
{
	return this->name;
}

/*--------------------------------------------------------- */
const char *clish_command__get_text(const clish_command_t * this)
{
	return this->text;
}

/*--------------------------------------------------------- */
void clish_command__set_action(clish_command_t * this, const char *action)
{
	assert(NULL == this->action);
	this->action = lub_string_dup(action);
}

/*--------------------------------------------------------- */
const char *clish_command__get_detail(const clish_command_t * this)
{
	return this->detail;
}

/*--------------------------------------------------------- */
void clish_command__set_detail(clish_command_t * this, const char *detail)
{
	assert(NULL == this->detail);
	this->detail = lub_string_dup(detail);
}

/*--------------------------------------------------------- */
char *clish_command__get_action(const clish_command_t * this,
				const char *viewid, clish_pargv_t * pargv)
{
	return clish_variable_expand(this->action, viewid, this, pargv);
}

/*--------------------------------------------------------- */
void clish_command__set_view(clish_command_t * this, clish_view_t * view)
{
	assert(NULL == this->view);
	this->view = view;
}

/*--------------------------------------------------------- */
clish_view_t *clish_command__get_view(const clish_command_t * this)
{
	return this->view;
}

/*--------------------------------------------------------- */
void clish_command__set_viewid(clish_command_t * this, const char *viewid)
{
	assert(NULL == this->viewid);
	this->viewid = lub_string_dup(viewid);
}

/*--------------------------------------------------------- */
char *clish_command__get_viewid(const clish_command_t * this,
				const char *viewid, clish_pargv_t * pargv)
{
	return clish_variable_expand(this->viewid, viewid, this, pargv);
}

/*--------------------------------------------------------- */
const clish_param_t *clish_command__get_param(const clish_command_t * this,
					      unsigned index)
{
	return clish_paramv__get_param(this->paramv, index);
}

/*--------------------------------------------------------- */
const char *clish_command__get_suffix(const clish_command_t * this)
{
	return lub_string_suffix(this->name);
}

/*--------------------------------------------------------- */
void clish_command__set_builtin(clish_command_t * this, const char *builtin)
{
	assert(NULL == this->builtin);
	this->builtin = lub_string_dup(builtin);
}

/*--------------------------------------------------------- */
const char *clish_command__get_builtin(const clish_command_t * this)
{
	return this->builtin;
}

/*--------------------------------------------------------- */
void
clish_command__set_escape_chars(clish_command_t * this,
				const char *escape_chars)
{
	assert(NULL == this->escape_chars);
	this->escape_chars = lub_string_dup(escape_chars);
}

/*--------------------------------------------------------- */
const char *clish_command__get_escape_chars(const clish_command_t * this)
{
	return this->escape_chars;
}

/*--------------------------------------------------------- */
void clish_command__set_args(clish_command_t * this, clish_param_t * args)
{
	assert(NULL == this->args);
	this->args = args;
}

/*--------------------------------------------------------- */
const clish_param_t *clish_command__get_args(const clish_command_t * this)
{
	return this->args;
}

/*--------------------------------------------------------- */
const unsigned clish_command__get_param_count(const clish_command_t * this)
{
	return clish_paramv__get_count(this->paramv);
}

/*--------------------------------------------------------- */
clish_paramv_t *clish_command__get_paramv(const clish_command_t * this)
{
	return this->paramv;
}

/*--------------------------------------------------------- */
void clish_command__set_pview(clish_command_t * this, clish_view_t * view)
{
	this->pview = view;
}

/*--------------------------------------------------------- */
clish_view_t *clish_command__get_pview(const clish_command_t * this)
{
	return this->pview;
}

/*--------------------------------------------------------- */
unsigned clish_command__get_depth(const clish_command_t * this)
{
	if (!this->pview)
		return 0;
	return clish_view__get_depth(this->pview);
}

/*--------------------------------------------------------- */
void
clish_command__set_cfg_op(clish_command_t * this,
			  clish_config_operation_t operation)
{
	this->cfg_op = operation;
}

/*--------------------------------------------------------- */
clish_config_operation_t clish_command__get_cfg_op(const clish_command_t * this)
{
	return this->cfg_op;
}

/*--------------------------------------------------------- */
void clish_command__set_priority(clish_command_t * this, unsigned short priority)
{
	this->priority = priority;
}

/*--------------------------------------------------------- */
unsigned short clish_command__get_priority(const clish_command_t * this)
{
	return this->priority;
}

/*--------------------------------------------------------- */
void clish_command__set_pattern(clish_command_t * this, const char *pattern)
{
	assert(NULL == this->pattern);
	this->pattern = lub_string_dup(pattern);
}

/*--------------------------------------------------------- */
char *clish_command__get_pattern(const clish_command_t * this,
				 clish_pargv_t * pargv)
{
	return clish_variable_expand(this->pattern, this->viewid, this, pargv);
}

/*--------------------------------------------------------- */
void clish_command__set_file(clish_command_t * this, const char *file)
{
	assert(NULL == this->file);
	this->file = lub_string_dup(file);
}

/*--------------------------------------------------------- */
char *clish_command__get_file(const clish_command_t * this,
			      clish_pargv_t * pargv)
{
	return clish_variable_expand(this->file, this->viewid, this, pargv);
}

/*--------------------------------------------------------- */
bool_t clish_command__get_splitter(const clish_command_t * this)
{
	return this->splitter;
}

/*--------------------------------------------------------- */
void clish_command__set_splitter(clish_command_t * this, bool_t splitter)
{
	this->splitter = splitter;
}


