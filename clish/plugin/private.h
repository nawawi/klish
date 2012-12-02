/*
 * plugin private.h
 */

#include "lub/list.h"
#include "clish/plugin.h"

/*---------------------------------------------------------
 * PRIVATE TYPES
 *--------------------------------------------------------- */

struct clish_sym_s {
	char *name; /* Symbol name */
	clish_plugin_fn_t *func; /* Function address */
};

struct clish_plugin_s {
	char *file; /* Plugin file name. Must be unique. */
	char *name; /* Local plugin name. Can be used in builtin ref. */
	lub_list_t *syms; /* List of plugin symbols */
};
