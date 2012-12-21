/*
 * plugin.h
 */
#ifndef _clish_plugin_h
#define _clish_plugin_h

/* Symbol types */

typedef struct clish_sym_s clish_sym_t;
typedef struct clish_plugin_s clish_plugin_t;

/* Plugin types */

typedef int clish_plugin_fn_t(void *context, char **out);
typedef int clish_plugin_init_t(clish_plugin_t *plugin);

/* Name of init function within plugin */
#define CLISH_PLUGIN_INIT "clish_plugin_init"

/* Symbol */

int clish_sym_compare(const void *first, const void *second);
clish_sym_t *clish_sym_new(const char *name, clish_plugin_fn_t *func);
void clish_sym_free(clish_sym_t *instance);
void clish_sym__set_func(clish_sym_t *instance, clish_plugin_fn_t *func);
clish_plugin_fn_t *clish_sym__get_func(clish_sym_t *instance);
char *clish_sym__get_name(clish_sym_t *instance);

/* Plugin */

clish_plugin_t *clish_plugin_new(const char *name, const char *file);
void clish_plugin_free(clish_plugin_t *instance);
void *clish_plugin_load(clish_plugin_t *instance);
clish_plugin_fn_t *clish_plugin_get_sym(clish_plugin_t *instance,
	const char *name);
int clish_plugin_add_sym(clish_plugin_t *instance,
	clish_plugin_fn_t *func, const char *name);
void clish_plugin_dump(const clish_plugin_t *instance);
char *clish_plugin__get_name(const clish_plugin_t *instance);
char *clish_plugin__get_file(const clish_plugin_t *instance);

#endif				/* _clish_plugin_h */
/** @} clish_plugin */
