/*
 * command.h
 */
#include "clish/command.h"

/*---------------------------------------------------------
 * PRIVATE TYPES
 *--------------------------------------------------------- */
struct clish_command_s {
	lub_bintree_node_t bt_node;
	char *name;
	char *text;
	clish_paramv_t *paramv;
	char *action;
	clish_view_t *view;
	char *viewid;
	char *detail;
	char *builtin;
	char *escape_chars;
	clish_param_t *args;
	bool_t link;
	clish_view_t *pview;
	clish_config_operation_t cfg_op;
	unsigned short priority;
	char *pattern;
	char *file;
	bool_t splitter;
};
