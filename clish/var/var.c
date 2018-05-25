/*
 * var.c
 *
 * This file provides the implementation of the "var" class
 */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "lub/string.h"
#include "private.h"

/*--------------------------------------------------------- */
static void clish_var_init(clish_var_t *this, const char *name)
{
	this->name = lub_string_dup(name);
	this->dynamic = BOOL_FALSE;
	this->value = NULL;
	this->action = clish_action_new();
	this->saved = NULL;

	/* Be a good binary tree citizen */
	lub_bintree_node_init(&this->bt_node);
}

/*--------------------------------------------------------- */
static void clish_var_fini(clish_var_t *this)
{
	lub_string_free(this->name);
	lub_string_free(this->value);
	lub_string_free(this->saved);
	clish_action_delete(this->action);
}

/*--------------------------------------------------------- */
int clish_var_bt_compare(const void *clientnode, const void *clientkey)
{
	const clish_var_t *this = clientnode;
	const char *key = clientkey;

	return strcmp(this->name, key);
}

/*-------------------------------------------------------- */
void clish_var_bt_getkey(const void *clientnode, lub_bintree_key_t * key)
{
	const clish_var_t *this = clientnode;

	/* fill out the opaque key */
	strcpy((char *)key, this->name);
}

/*--------------------------------------------------------- */
size_t clish_var_bt_offset(void)
{
	return offsetof(clish_var_t, bt_node);
}

/*--------------------------------------------------------- */
clish_var_t *clish_var_new(const char *name)
{
	clish_var_t *this = malloc(sizeof(clish_var_t));
	if (this)
		clish_var_init(this, name);
	return this;
}

/*--------------------------------------------------------- */
void clish_var_delete(clish_var_t *this)
{
	clish_var_fini(this);
	free(this);
}

CLISH_GET_STR(var, name);
CLISH_SET(var, bool_t, dynamic);
CLISH_GET(var, bool_t, dynamic);
CLISH_SET_STR(var, value);
CLISH_GET_STR(var, value);
CLISH_SET_STR(var, saved);
CLISH_GET_STR(var, saved);
CLISH_GET(var, clish_action_t *, action);
