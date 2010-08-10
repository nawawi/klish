/*
 * conf.h
 */
#ifndef buf_private_h
#define buf_private_h

#include "cliconf/buf.h"
#include "lub/bintree.h"

/*---------------------------------------------------------
 * PRIVATE TYPES
 *--------------------------------------------------------- */
struct conf_buf_s {
	lub_bintree_node_t bt_node;
	int sock;
	int size;
	char *buf;
	int pos;
	int rpos;
};

#endif
