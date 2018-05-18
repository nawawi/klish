/*
  * config.c
  *
  * This file provides the implementation of a config definition
  */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "lub/types.h"
#include "lub/string.h"
#include "private.h"

/*--------------------------------------------------------- */
static void clish_config_init(clish_config_t *this)
{
	this->op = CLISH_CONFIG_NONE;
	this->priority = 0;
	this->pattern = NULL;
	this->file = NULL;
	this->splitter = BOOL_TRUE;
	this->seq = NULL;
	this->unique = BOOL_TRUE;
	this->depth = NULL;
}

/*--------------------------------------------------------- */
static void clish_config_fini(clish_config_t *this)
{
	lub_string_free(this->pattern);
	lub_string_free(this->file);
	lub_string_free(this->seq);
	lub_string_free(this->depth);
}

/*--------------------------------------------------------- */
clish_config_t *clish_config_new(void)
{
	clish_config_t *this = malloc(sizeof(clish_config_t));

	if (this)
		clish_config_init(this);

	return this;
}

/*--------------------------------------------------------- */
void clish_config_delete(clish_config_t *this)
{
	clish_config_fini(this);
	free(this);
}

CLISH_SET(config, clish_config_op_e, op);
CLISH_GET(config, clish_config_op_e, op);
CLISH_SET(config, unsigned short, priority);
CLISH_GET(config, unsigned short, priority);
CLISH_SET(config, bool_t, splitter);
CLISH_GET(config, bool_t, splitter);
CLISH_SET(config, bool_t, unique);
CLISH_GET(config, bool_t, unique);
CLISH_SET_STR_ONCE(config, pattern);
CLISH_GET_STR(config, pattern);
CLISH_SET_STR_ONCE(config, file);
CLISH_GET_STR(config, file);
CLISH_SET_STR_ONCE(config, seq);
CLISH_GET_STR(config, seq);
CLISH_SET_STR_ONCE(config, depth);
CLISH_GET_STR(config, depth);
