/*
 * view.h
 */
#include "clish/view.h"
#include "lub/bintree.h"

/*---------------------------------------------------------
 * PRIVATE TYPES
 *--------------------------------------------------------- */
struct clish_view_s {
	lub_bintree_t tree;
	lub_bintree_node_t bt_node;
	char *name;
	char *prompt;
	unsigned nspacec;
	clish_nspace_t **nspacev;
	unsigned depth;
};
