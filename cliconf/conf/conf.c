/*
 * conf.c
 *
 * This file provides the implementation of a conf class
 */
#include "private.h"
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
int cliconf_bt_compare(const void *clientnode, const void *clientkey)
{
	const cliconf_t *this = clientnode;
	unsigned short *pri = (unsigned short *)clientkey;
	char *line = ((char *)clientkey + sizeof(unsigned short));

/*	printf("COMPARE: node_pri=%d node_line=[%s] key_pri=%d key_line=[%s]\n",
	cliconf__get_priority(this), this->line, *pri, line);
*/
	if (cliconf__get_priority(this) == *pri)
		return lub_string_nocasecmp(this->line, line);

	return (cliconf__get_priority(this) - *pri);
}

/*-------------------------------------------------------- */
static void cliconf_key(lub_bintree_key_t * key,
	unsigned short priority, const char *text)
{
	unsigned short *pri = (unsigned short *)key;
	char *line = ((char *)key + sizeof(unsigned short));

	/* fill out the opaque key */
	*pri = priority;
	strcpy(line, text);
}

/*-------------------------------------------------------- */
void cliconf_bt_getkey(const void *clientnode, lub_bintree_key_t * key)
{
	const cliconf_t *this = clientnode;

	cliconf_key(key, cliconf__get_priority(this), this->line);
}

/*---------------------------------------------------------
 * PRIVATE METHODS
 *--------------------------------------------------------- */
static void
cliconf_init(cliconf_t * this, const char *line, unsigned short priority)
{
	/* set up defaults */
	this->line = lub_string_dup(line);
	this->priority = priority;
	this->splitter = BOOL_TRUE;

	/* Be a good binary tree citizen */
	lub_bintree_node_init(&this->bt_node);

	/* initialise the tree of commands for this conf */
	lub_bintree_init(&this->tree,
		cliconf_bt_offset(),
		 cliconf_bt_compare, cliconf_bt_getkey);
}

/*--------------------------------------------------------- */
static void cliconf_fini(cliconf_t * this)
{
	cliconf_t *conf;

	/* delete each conf held by this conf */
	while ((conf = lub_bintree_findfirst(&this->tree))) {
		/* remove the conf from the tree */
		lub_bintree_remove(&this->tree, conf);
		/* release the instance */
		cliconf_delete(conf);
	}

	/* free our memory */
	lub_string_free(this->line);
	this->line = NULL;
}

/*---------------------------------------------------------
 * PUBLIC META FUNCTIONS
 *--------------------------------------------------------- */
size_t cliconf_bt_offset(void)
{
	return offsetof(cliconf_t, bt_node);
}

/*--------------------------------------------------------- */
cliconf_t *cliconf_new(const char *line, unsigned short priority)
{
	cliconf_t *this = malloc(sizeof(cliconf_t));

	if (this) {
		cliconf_init(this, line, priority);
	}

	return this;
}

/*---------------------------------------------------------
 * PUBLIC METHODS
 *--------------------------------------------------------- */
void cliconf_delete(cliconf_t * this)
{
	cliconf_fini(this);
	free(this);
}

void cliconf_fprintf(cliconf_t * this, FILE * stream,
		const char *pattern, int depth,
		unsigned char prev_pri_hi)
{
	cliconf_t *conf;
	lub_bintree_iterator_t iter;
	unsigned char pri = 0;

	if (this->line && *(this->line) != '\0') {
		char *space = NULL;

		if (depth > 0) {
			space = malloc(depth + 1);
			memset(space, ' ', depth);
			space[depth] = '\0';
		}
		if ((0 == depth) &&
			(this->splitter ||
			(cliconf__get_priority_hi(this) != prev_pri_hi)))
			fprintf(stream, "!\n");
		fprintf(stream, "%s%s\n", space ? space : "", this->line);
		free(space);
	}

	/* iterate child elements */
	if (!(conf = lub_bintree_findfirst(&this->tree)))
		return;

	for(lub_bintree_iterator_init(&iter, &this->tree, conf);
		conf; conf = lub_bintree_iterator_next(&iter)) {
		if (pattern &&
			(lub_string_nocasestr(conf->line, pattern) != conf->line))
			continue;
		cliconf_fprintf(conf, stream, NULL, depth + 1, pri);
		pri = cliconf__get_priority_hi(conf);
	}
}

/*--------------------------------------------------------- */
cliconf_t *cliconf_new_conf(cliconf_t * this,
					const char *line, unsigned short priority)
{
	/* allocate the memory for a new child element */
	cliconf_t *conf = cliconf_new(line, priority);
	assert(conf);

	/* ...insert it into the binary tree for this conf */
	if (-1 == lub_bintree_insert(&this->tree, conf)) {
		/* inserting a duplicate command is bad */
		cliconf_delete(conf);
		conf = NULL;
	}

	return conf;
}

/*--------------------------------------------------------- */
cliconf_t *cliconf_find_conf(cliconf_t * this,
	const char *line, unsigned short priority)
{
	cliconf_t *conf;
	lub_bintree_key_t key;
	lub_bintree_iterator_t iter;

	if (0 != priority) {
		cliconf_key(&key, priority, line);
		return lub_bintree_find(&this->tree, &key);
	}

	/* Empty tree */
	if (!(conf = lub_bintree_findfirst(&this->tree)))
		return NULL;

	/* Iterate non-empty tree */
	lub_bintree_iterator_init(&iter, &this->tree, conf);
	while ((conf = lub_bintree_iterator_next(&iter))) {
		if (0 == lub_string_nocasecmp(conf->line, line))
			return conf;
	}

	return NULL;
}

/*--------------------------------------------------------- */
void cliconf_del_pattern(cliconf_t *this,
	const char *pattern)
{
	cliconf_t *conf;
	lub_bintree_iterator_t iter;

	/* Empty tree */
	if (!(conf = lub_bintree_findfirst(&this->tree)))
		return;

	lub_bintree_iterator_init(&iter, &this->tree, conf);
	while ((conf = lub_bintree_iterator_next(&iter))) {
		if (lub_string_nocasestr(conf->line, pattern) == conf->line) {
			lub_bintree_remove(&this->tree, conf);
			cliconf_delete(conf);
		}
	}
}

/*--------------------------------------------------------- */
unsigned short cliconf__get_priority(const cliconf_t * this)
{
	return this->priority;
}

/*--------------------------------------------------------- */
unsigned char cliconf__get_priority_hi(const cliconf_t * this)
{
	return (unsigned char)(this->priority >> 8);
}

/*--------------------------------------------------------- */
unsigned char cliconf__get_priority_lo(const cliconf_t * this)
{
	return (unsigned char)(this->priority & 0xff);
}

/*--------------------------------------------------------- */
bool_t cliconf__get_splitter(const cliconf_t * this)
{
	return this->splitter;
}

/*--------------------------------------------------------- */
void cliconf__set_splitter(cliconf_t *this, bool_t splitter)
{
	this->splitter = splitter;
}

