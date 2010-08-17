/*
 * conf.h
 */
#ifndef _konf_tree_private_h
#define _konf_tree_private_h

#include "konf/tree.h"
#include "lub/types.h"
#include "lub/bintree.h"

/*---------------------------------------------------------
 * PRIVATE TYPES
 *--------------------------------------------------------- */
struct konf_tree_s {
	lub_bintree_t tree;
	lub_bintree_node_t bt_node;
	char *line;
	unsigned short priority;
	bool_t splitter;
};

#endif
