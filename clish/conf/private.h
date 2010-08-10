/*
 * conf.h
 */
#include "clish/conf.h"
#include "lub/bintree.h"

/*---------------------------------------------------------
 * PRIVATE TYPES
 *--------------------------------------------------------- */
struct clish_conf_s {
	lub_bintree_t tree;
	lub_bintree_node_t bt_node;
	char *line;
	clish_command_t *cmd;
};
