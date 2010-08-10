/*
 * conf.c
 *
 * This file provides the implementation of a conf class
 */
#include "private.h"
#include "clish/variable.h"
#include "lub/argv.h"
#include "lub/string.h"
#include "lub/ctype.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*---------------------------------------------------------
 * PRIVATE META FUNCTIONS
 *--------------------------------------------------------- */
int clish_conf_bt_compare(const void *clientnode, const void *clientkey)
{
	const clish_conf_t *this = clientnode;
	char *pri = (char *)clientkey;
	char *line = ((char *)clientkey + 1);

	if ((clish_conf__get_priority(this) == *pri) || (0 == *pri))
		return lub_string_nocasecmp(this->line, line);

	return (clish_conf__get_priority(this) - *pri);
}

/*-------------------------------------------------------- */
static void clish_conf_key(lub_bintree_key_t * key, char priority, char *text)
{
	char *pri = (char *)key;
	char *line = ((char *)key + 1);

	/* fill out the opaque key */
	*pri = priority;
	strcpy(line, text);
}

/*-------------------------------------------------------- */
void clish_conf_bt_getkey(const void *clientnode, lub_bintree_key_t * key)
{
	const clish_conf_t *this = clientnode;

	clish_conf_key(key, clish_conf__get_priority(this), this->line);
}

/*---------------------------------------------------------
 * PRIVATE METHODS
 *--------------------------------------------------------- */
static void
clish_conf_init(clish_conf_t * this, const char *line,
		const clish_command_t * cmd)
{
	/* set up defaults */
	this->line = lub_string_dup(line);
	this->cmd = cmd;

	/* Be a good binary tree citizen */
	lub_bintree_node_init(&this->bt_node);

	/* initialise the tree of commands for this conf */
	lub_bintree_init(&this->tree,
			 clish_conf_bt_offset(),
			 clish_conf_bt_compare, clish_conf_bt_getkey);
}

/*--------------------------------------------------------- */
static void clish_conf_fini(clish_conf_t * this)
{
	clish_conf_t *conf;
	unsigned i;

	/* delete each conf held by this conf */
	while ((conf = lub_bintree_findfirst(&this->tree))) {
		/* remove the conf from the tree */
		lub_bintree_remove(&this->tree, conf);

		/* release the instance */
		clish_conf_delete(conf);
	}

	/* free our memory */
	lub_string_free(this->line);
	this->line = NULL;
}

/*---------------------------------------------------------
 * PUBLIC META FUNCTIONS
 *--------------------------------------------------------- */
size_t clish_conf_bt_offset(void)
{
	return offsetof(clish_conf_t, bt_node);
}

/*--------------------------------------------------------- */
clish_conf_t *clish_conf_new(const char *line, const clish_command_t * cmd)
{
	clish_conf_t *this = malloc(sizeof(clish_conf_t));

	if (this) {
		clish_conf_init(this, line, cmd);
	}

	return this;
}

/*---------------------------------------------------------
 * PUBLIC METHODS
 *--------------------------------------------------------- */
void clish_conf_delete(clish_conf_t * this)
{
	clish_conf_fini(this);
	free(this);
}

void clish_conf_fprintf(FILE * stream, clish_conf_t * this)
{
	clish_conf_t *conf;
	lub_bintree_iterator_t iter;

	if (this->cmd && this->line) {
		char *space;
		int depth = clish_command__get_depth(this->cmd);
		space = malloc(depth + 1);
		memset(space, ' ', depth);
		space[depth] = '\0';
		fprintf(stream, "%s%s\n", space, this->line);
		free(space);
	}

	/* iterate child elements */
	if (conf = lub_bintree_findfirst(&this->tree)) {
		for (lub_bintree_iterator_init(&iter, &this->tree, conf);
		     conf; conf = lub_bintree_iterator_next(&iter)) {
			if (clish_conf__get_depth(conf) == 0)
				fprintf(stream, "!\n");
			clish_conf_fprintf(stream, conf);
		}
	}
}

/*--------------------------------------------------------- */
clish_conf_t *clish_conf_new_conf(clish_conf_t * this,
				  const char *line, const clish_command_t * cmd)
{
	/* allocate the memory for a new child element */
	clish_conf_t *conf = clish_conf_new(line, cmd);
	assert(conf);

	/* ...insert it into the binary tree for this conf */
	if (-1 == lub_bintree_insert(&this->tree, conf)) {
		/* inserting a duplicate command is bad */
		clish_conf_delete(conf);
		conf = NULL;
	}

	return conf;
}

/*--------------------------------------------------------- */
clish_conf_t *clish_conf_find_conf(clish_conf_t * this,
				   const char *line,
				   const clish_command_t * cmd)
{
	lub_bintree_key_t key;
	char priority = 0;

	if (cmd)
		priority = clish_command__get_priority(cmd);
	clish_conf_key(&key, priority, line);

	return lub_bintree_find(&this->tree, &key);
}

/*--------------------------------------------------------- */
unsigned clish_conf__get_depth(const clish_conf_t * this)
{
	if (!this->cmd)
		return 0;

	return clish_command__get_depth(this->cmd);
}

/*--------------------------------------------------------- */
char clish_conf__get_priority(const clish_conf_t * this)
{
	if (!this->cmd)
		return 0;

	return clish_command__get_priority(this->cmd);
}
