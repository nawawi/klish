/*
 * conf.h
 */
#ifndef conf_private_h
#define conf_private_h

#include "cliconf/conf.h"
#include "lub/types.h"
#include "lub/bintree.h"

/*---------------------------------------------------------
 * PRIVATE TYPES
 *--------------------------------------------------------- */
struct cliconf_s {
	lub_bintree_t tree;
	lub_bintree_node_t bt_node;
	char *line;
	unsigned short priority;
	bool_t splitter;
};

#endif
