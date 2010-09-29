/*
 * nspace.h
 */
#include "clish/nspace.h"

/*---------------------------------------------------------
 * PRIVATE TYPES
 *--------------------------------------------------------- */
struct clish_nspace_s {
	clish_view_t *view;	/* The view to import commands from */
	char *prefix;		/* if non NULL the prefix for imported commands */
	clish_command_t * prefix_cmd;
	clish_command_t * proxy_cmd;
	bool_t help;
	bool_t completion;
	bool_t context_help;
	bool_t inherit;
};
