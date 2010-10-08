/*
 * tree.c
 *
 * This file provides the implementation of a konf_tree class
 */

#include "private.h"
#include "lub/argv.h"
#include "lub/string.h"
#include "lub/ctype.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <regex.h>

/*---------------------------------------------------------
 * PRIVATE META FUNCTIONS
 *--------------------------------------------------------- */
int konf_tree_bt_compare(const void *clientnode, const void *clientkey)
{
	const konf_tree_t *this = clientnode;
	unsigned short *pri = (unsigned short *)clientkey;
	unsigned short *seq = (unsigned short *)clientkey + 1;
	unsigned short *sub = (unsigned short *)clientkey + 2;
	char *line = ((char *)clientkey + (3 * sizeof(unsigned short)));

	/* Priority check */
	if (this->priority != *pri)
		return (this->priority - *pri);
	/* Sequence check */
	if (this->seq_num != *seq)
		return (this->seq_num - *seq);
	/* Sub-sequence check */
	if (this->sub_num != *sub)
		return (this->sub_num - *sub);
	/* Line check */
	return lub_string_nocasecmp(this->line, line);
}

/*-------------------------------------------------------- */
static void konf_tree_key(lub_bintree_key_t * key,
	unsigned short priority, unsigned short sequence,
	unsigned short subseq, const char *text)
{
	unsigned short *pri = (unsigned short *)key;
	unsigned short *seq = (unsigned short *)key + 1;
	unsigned short *sub = (unsigned short *)key + 2;
	char *line = ((char *)key + (3 * sizeof(unsigned short)));

	/* fill out the opaque key */
	*pri = priority;
	*seq = sequence;
	*sub = subseq;
	strcpy(line, text);
}

/*-------------------------------------------------------- */
void konf_tree_bt_getkey(const void *clientnode, lub_bintree_key_t * key)
{
	const konf_tree_t *this = clientnode;

	konf_tree_key(key, this->priority, this->seq_num,
		this->sub_num, this->line);
}

/*---------------------------------------------------------
 * PRIVATE METHODS
 *--------------------------------------------------------- */
static void
konf_tree_init(konf_tree_t * this, const char *line, unsigned short priority)
{
	/* set up defaults */
	this->line = lub_string_dup(line);
	this->priority = priority;
	this->seq_num = 0;
	this->sub_num = KONF_ENTRY_OK;
	this->splitter = BOOL_TRUE;

	/* Be a good binary tree citizen */
	lub_bintree_node_init(&this->bt_node);

	/* initialise the tree of commands for this conf */
	lub_bintree_init(&this->tree,
		konf_tree_bt_offset(),
		 konf_tree_bt_compare, konf_tree_bt_getkey);
}

/*--------------------------------------------------------- */
static void konf_tree_fini(konf_tree_t * this)
{
	konf_tree_t *conf;

	/* delete each conf held by this conf */
	while ((conf = lub_bintree_findfirst(&this->tree))) {
		/* remove the conf from the tree */
		lub_bintree_remove(&this->tree, conf);
		/* release the instance */
		konf_tree_delete(conf);
	}

	/* free our memory */
	lub_string_free(this->line);
	this->line = NULL;
}

/*---------------------------------------------------------
 * PUBLIC META FUNCTIONS
 *--------------------------------------------------------- */
size_t konf_tree_bt_offset(void)
{
	return offsetof(konf_tree_t, bt_node);
}

/*--------------------------------------------------------- */
konf_tree_t *konf_tree_new(const char *line, unsigned short priority)
{
	konf_tree_t *this = malloc(sizeof(konf_tree_t));

	if (this) {
		konf_tree_init(this, line, priority);
	}

	return this;
}

/*---------------------------------------------------------
 * PUBLIC METHODS
 *--------------------------------------------------------- */
void konf_tree_delete(konf_tree_t * this)
{
	konf_tree_fini(this);
	free(this);
}

/*--------------------------------------------------------- */
void konf_tree_fprintf(konf_tree_t * this, FILE * stream,
		const char *pattern, int depth,
		bool_t seq, unsigned char prev_pri_hi)
{
	konf_tree_t *conf;
	lub_bintree_iterator_t iter;
	unsigned char pri = 0;
	regex_t regexp;

	if (this->line && *(this->line) != '\0') {
		char *space = NULL;

		if (depth > 0) {
			space = malloc(depth + 1);
			memset(space, ' ', depth);
			space[depth] = '\0';
		}
		if ((0 == depth) &&
			(this->splitter ||
			(konf_tree__get_priority_hi(this) != prev_pri_hi)))
			fprintf(stream, "!\n");
		fprintf(stream, "%s", space ? space : "");
		if (seq && (konf_tree__get_seq_num(this) != 0))
			fprintf(stream, "%02u ", konf_tree__get_seq_num(this));
		fprintf(stream, "%s\n", this->line);
		free(space);
	}

	/* regexp compilation */
	if (pattern)
		regcomp(&regexp, pattern, REG_EXTENDED | REG_ICASE);

	/* iterate child elements */
	if (!(conf = lub_bintree_findfirst(&this->tree)))
		return;

	for(lub_bintree_iterator_init(&iter, &this->tree, conf);
		conf; conf = lub_bintree_iterator_next(&iter)) {
		if (pattern && (0 != regexec(&regexp, conf->line, 0, NULL, 0)))
			continue;
		konf_tree_fprintf(conf, stream, NULL, depth + 1, seq, pri);
		pri = konf_tree__get_priority_hi(conf);
	}
	if (pattern)
		regfree(&regexp);
}

static int normalize_seq(konf_tree_t * this, unsigned short priority)
{
	unsigned short cnt = 1;
	konf_tree_t *conf = NULL;
	lub_bintree_iterator_t iter;

	/* If tree is empty */
	if (!(conf = lub_bintree_findfirst(&this->tree)))
		return 0;

	/* Iterate and set dirty */
	lub_bintree_iterator_init(&iter, &this->tree, conf);
	do {
		unsigned short cur_pri = konf_tree__get_priority(conf);
		if (cur_pri < priority)
			continue;
		if (konf_tree__get_seq_num(conf) == 0)
			continue;
		if (cur_pri > priority)
			break;
		if (konf_tree__get_sub_num(conf) == KONF_ENTRY_OK)
			konf_tree__set_sub_num(conf, KONF_ENTRY_DIRTY);
	} while ((conf = lub_bintree_iterator_next(&iter)));

	/* Iterate and renum */
	conf = lub_bintree_findfirst(&this->tree);
	lub_bintree_iterator_init(&iter, &this->tree, conf);
	do {
		unsigned short cur_pri = konf_tree__get_priority(conf);
		if (cur_pri < priority)
			continue;
		if (konf_tree__get_seq_num(conf) == 0)
			continue;
		if (cur_pri > priority)
			break;
		if (konf_tree__get_sub_num(conf) == KONF_ENTRY_OK)
			continue;
		lub_bintree_remove(&this->tree, conf);
		konf_tree__set_sub_num(conf, KONF_ENTRY_OK);
		konf_tree__set_seq_num(conf, cnt++);
		lub_bintree_insert(&this->tree, conf);
	} while ((conf = lub_bintree_iterator_next(&iter)));

	return 0;
}

/*--------------------------------------------------------- */
konf_tree_t *konf_tree_new_conf(konf_tree_t * this,
	const char *line, unsigned short priority,
	bool_t seq, unsigned short seq_num)
{
	/* Allocate the memory for a new child element */
	konf_tree_t *newconf = konf_tree_new(line, priority);
	assert(newconf);

	/* Sequence */
	if (seq) {
		konf_tree__set_seq_num(newconf,
			seq_num ? seq_num : 0xffff);
		konf_tree__set_sub_num(newconf, KONF_ENTRY_NEW);
	}

	/* Insert it into the binary tree for this conf */
	if (-1 == lub_bintree_insert(&this->tree, newconf)) {
		/* inserting a duplicate command is bad */
		konf_tree_delete(newconf);
		newconf = NULL;
	}

	if (seq)
		normalize_seq(this, priority);

	return newconf;
}

/*--------------------------------------------------------- */
konf_tree_t *konf_tree_find_conf(konf_tree_t * this,
	const char *line, unsigned short priority, unsigned short seq_num)
{
	konf_tree_t *conf;
	lub_bintree_key_t key;
	lub_bintree_iterator_t iter;

	if ((0 != priority) && (0 != seq_num)) {
		konf_tree_key(&key, priority, seq_num,
			KONF_ENTRY_OK, line);
		return lub_bintree_find(&this->tree, &key);
	}

	/* If tree is empty */
	if (!(conf = lub_bintree_findfirst(&this->tree)))
		return NULL;

	/* Iterate non-empty tree */
	lub_bintree_iterator_init(&iter, &this->tree, conf);
	do {
		if (0 == lub_string_nocasecmp(conf->line, line))
			return conf;
	} while ((conf = lub_bintree_iterator_next(&iter)));

	return NULL;
}

/*--------------------------------------------------------- */
int konf_tree_del_pattern(konf_tree_t *this,
	const char *pattern, unsigned short priority,
	bool_t seq, unsigned short seq_num)
{
	konf_tree_t *conf;
	lub_bintree_iterator_t iter;
	regex_t regexp;
	int del_cnt = 0; /* how many strings were deleted */

	if (seq && (0 == priority))
		return -1;

	/* Is tree empty? */
	if (!(conf = lub_bintree_findfirst(&this->tree)))
		return 0;

	/* Compile regular expression */
	regcomp(&regexp, pattern, REG_EXTENDED | REG_ICASE);

	/* Iterate configuration tree */
	lub_bintree_iterator_init(&iter, &this->tree, conf);
	do {
		if (0 != regexec(&regexp, conf->line, 0, NULL, 0))
			continue;
		if ((0 != priority) &&
			(priority != conf->priority))
			continue;
		if (seq && (seq_num != 0) &&
			(seq_num != conf->seq_num))
			continue;
		if (seq && (0 == seq_num) && (0 == conf->seq_num))
			continue;
		lub_bintree_remove(&this->tree, conf);
		konf_tree_delete(conf);
		del_cnt++;
	} while ((conf = lub_bintree_iterator_next(&iter)));

	regfree(&regexp);

	if (seq && (del_cnt != 0))
		normalize_seq(this, priority);

	return 0;
}

/*--------------------------------------------------------- */
unsigned short konf_tree__get_priority(const konf_tree_t * this)
{
	return this->priority;
}

/*--------------------------------------------------------- */
unsigned char konf_tree__get_priority_hi(const konf_tree_t * this)
{
	return (unsigned char)(this->priority >> 8);
}

/*--------------------------------------------------------- */
unsigned char konf_tree__get_priority_lo(const konf_tree_t * this)
{
	return (unsigned char)(this->priority & 0xff);
}

/*--------------------------------------------------------- */
bool_t konf_tree__get_splitter(const konf_tree_t * this)
{
	return this->splitter;
}

/*--------------------------------------------------------- */
void konf_tree__set_splitter(konf_tree_t *this, bool_t splitter)
{
	this->splitter = splitter;
}

/*--------------------------------------------------------- */
unsigned short konf_tree__get_seq_num(const konf_tree_t * this)
{
	return this->seq_num;
}

/*--------------------------------------------------------- */
void konf_tree__set_seq_num(konf_tree_t * this, unsigned short seq_num)
{
	this->seq_num = seq_num;
}

/*--------------------------------------------------------- */
unsigned short konf_tree__get_sub_num(const konf_tree_t * this)
{
	return this->sub_num;
}

/*--------------------------------------------------------- */
void konf_tree__set_sub_num(konf_tree_t * this, unsigned short sub_num)
{
	this->sub_num = sub_num;
}

/*--------------------------------------------------------- */
const char * konf_tree__get_line(const konf_tree_t * this)
{
	return this->line;
}

