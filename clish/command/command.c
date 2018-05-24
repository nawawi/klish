/*
  * command.c
  *
  * This file provides the implementation of a command definition  
  */

#include "private.h"
#include "clish/types.h"
#include "lub/bintree.h"
#include "lub/string.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static void
clish_command_init(clish_command_t *this, const char *name, const char *text)
{
	/* initialise the node part */
	this->name = lub_string_dup(name);
	this->text = lub_string_dup(text);

	/* Be a good binary tree citizen */
	lub_bintree_node_init(&this->bt_node);

	/* set up defaults */
	this->link = NULL;
	this->alias = NULL;
	this->alias_view = NULL;
	this->paramv = clish_paramv_new();
	this->viewid = NULL;
	this->viewname = NULL;
	this->action = clish_action_new();
	this->config = clish_config_new();
	this->detail = NULL;
	this->escape_chars = NULL;
	this->regex_chars = NULL;
	this->args = NULL;
	this->pview = NULL;
	this->dynamic = BOOL_FALSE;
	this->internal = BOOL_FALSE;
	this->access = NULL;
}

/*--------------------------------------------------------- */
static void clish_command_fini(clish_command_t * this)
{
	lub_string_free(this->name);
	lub_string_free(this->text);

	/* Link need not full cleanup */
	if (this->link)
		return;

	/* finalize each of the parameter instances */
	clish_paramv_delete(this->paramv);

	clish_action_delete(this->action);
	clish_config_delete(this->config);
	lub_string_free(this->alias);
	lub_string_free(this->alias_view);
	lub_string_free(this->viewname);
	lub_string_free(this->viewid);
	lub_string_free(this->detail);
	lub_string_free(this->escape_chars);
	lub_string_free(this->regex_chars);
	lub_string_free(this->access);
	if (this->args)
		clish_param_delete(this->args);
}

/*--------------------------------------------------------- */
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
clish_command_t *clish_command_new(const char *name, const char *help)
{
	clish_command_t *this = malloc(sizeof(clish_command_t));

	if (this)
		clish_command_init(this, name, help);

	return this;
}

/*--------------------------------------------------------- */
clish_command_t *clish_command_new_link(const char *name,
	const char *help, const clish_command_t * ref)
{
	if (!ref)
		return NULL;

	clish_command_t *this = malloc(sizeof(clish_command_t));
	assert(this);

	/* Copy all fields to the new command-link */
	*this = *ref;
	/* Initialise the name (other than original name) */
	this->name = lub_string_dup(name);
	/* Initialise the help (other than original help) */
	this->text = lub_string_dup(help);
	/* Be a good binary tree citizen */
	lub_bintree_node_init(&this->bt_node);
	/* It a link to command so set the link flag */
	this->link = ref;

	return this;
}

/*--------------------------------------------------------- */
clish_command_t * clish_command_alias_to_link(clish_command_t *this, clish_command_t *ref)
{
	clish_command_t tmp;

	if (!this || !ref)
		return NULL;
	if (ref->alias) /* The reference is a link too */
		return NULL;
	memcpy(&tmp, this, sizeof(tmp));
	*this = *ref;
	memcpy(&this->bt_node, &tmp.bt_node, sizeof(tmp.bt_node));
	this->name = lub_string_dup(tmp.name); /* Save an original name */
	this->text = lub_string_dup(tmp.text); /* Save an original help */
	this->link = ref;
	this->pview = tmp.pview; /* Save an original parent view */
	clish_command_fini(&tmp);

	return this;
}

/*--------------------------------------------------------- */
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
int clish_command_help(const clish_command_t *this)
{
	this = this; /* Happy compiler */

	return 0;
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
int clish_command_diff(const clish_command_t * cmd1,
	const clish_command_t * cmd2)
{
	if (NULL == cmd1) {
		if (NULL != cmd2)
			return 1;
		else
			return 0;
	}
	if (NULL == cmd2)
		return -1;

	return lub_string_nocasecmp(clish_command__get_name(cmd1),
		clish_command__get_name(cmd2));
}

CLISH_GET_STR(command, name);
CLISH_GET_STR(command, text);
CLISH_SET_STR_ONCE(command, detail);
CLISH_GET_STR(command, detail);
CLISH_GET(command, clish_action_t *, action);
CLISH_GET(command, clish_config_t *, config);
CLISH_SET_STR_ONCE(command, regex_chars);
CLISH_GET_STR(command, regex_chars);
CLISH_SET_STR_ONCE(command, escape_chars);
CLISH_GET_STR(command, escape_chars);
CLISH_SET_STR_ONCE(command, viewname);
CLISH_GET_STR(command, viewname);
CLISH_SET_STR_ONCE(command, viewid);
CLISH_GET_STR(command, viewid);
CLISH_SET_ONCE(command, clish_param_t *, args);
CLISH_GET(command, clish_param_t *, args);
CLISH_GET(command, clish_paramv_t *, paramv);
CLISH_SET(command, clish_view_t *, pview);
CLISH_GET(command, clish_view_t *, pview);
CLISH_SET_STR(command, access);
CLISH_GET_STR(command, access);
CLISH_SET_STR(command, alias);
CLISH_GET_STR(command, alias);
CLISH_SET_STR(command, alias_view);
CLISH_GET_STR(command, alias_view);
CLISH_SET(command, bool_t, internal);
CLISH_GET(command, bool_t, internal);
CLISH_SET(command, bool_t, dynamic);
CLISH_GET(command, bool_t, dynamic);

/*--------------------------------------------------------- */
void clish_command__force_viewname(clish_command_t * this, const char *viewname)
{
	if (this->viewname)
		lub_string_free(this->viewname);
	this->viewname = lub_string_dup(viewname);
}

/*--------------------------------------------------------- */
void clish_command__force_viewid(clish_command_t * this, const char *viewid)
{
	if (this->viewid)
		lub_string_free(this->viewid);
	this->viewid = lub_string_dup(viewid);
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
unsigned int clish_command__get_param_count(const clish_command_t * this)
{
	return clish_paramv__get_count(this->paramv);
}

/*--------------------------------------------------------- */
int clish_command__get_depth(const clish_command_t * this)
{
	if (!this->pview)
		return 0;
	return clish_view__get_depth(this->pview);
}

/*--------------------------------------------------------- */
clish_view_restore_e clish_command__get_restore(const clish_command_t * this)
{
	if (!this->pview)
		return CLISH_RESTORE_NONE;
	return clish_view__get_restore(this->pview);
}

/*--------------------------------------------------------- */
const clish_command_t * clish_command__get_orig(const clish_command_t * this)
{
	if (this->link)
		return clish_command__get_orig(this->link);
	return this;
}

/*--------------------------------------------------------- */
const clish_command_t * clish_command__get_cmd(const clish_command_t * this)
{
	if (!this->dynamic)
		return this;
	if (this->link)
		return clish_command__get_cmd(this->link);
	return NULL;
}
