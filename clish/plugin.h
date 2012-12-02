/*
 * plugin.h
 */
#ifndef _clish_plugin_h
#define _clish_plugin_h

#include "clish/shell.h"

typedef struct clish_sym_s clish_sym_t;
typedef struct clish_plugin_s clish_plugin_t;

typedef int clish_plugin_fn_t(clish_context_t *context, char **out);
typedef int clish_plugin_init_t(clish_plugin_t *plugin);

/* Name of init function within plugin */
#define CLISH_PLUGIN_INIT "clish_plugin_init"

clish_plugin_t *clish_plugin_new(const char *file);
void clish_plugin_free(clish_plugin_t *instance);
int clish_plugin_load(clish_plugin_t *instance);

int clish_plugin_sym(clish_plugin_t *instance,
	clish_plugin_fn_t *func, const char *name);

#endif				/* _clish_plugin_h */
/** @} clish_plugin */
