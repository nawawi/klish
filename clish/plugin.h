/*
 * plugin.h
 */
#ifndef _clish_plugin_h
#define _clish_plugin_h

/* Symbol types */

typedef struct clish_sym_s clish_sym_t;
typedef struct clish_plugin_s clish_plugin_t;

/* Plugin types */

/* Name of init function within plugin */
#define CLISH_PLUGIN_INIT_FNAME clish_plugin_init
#define CLISH_PLUGIN_INIT_NAME "clish_plugin_init"
#define CLISH_PLUGIN_INIT_FUNC(name) int name(clish_plugin_t *plugin)
#define CLISH_PLUGIN_INIT CLISH_PLUGIN_INIT_FUNC(CLISH_PLUGIN_INIT_FNAME)

#define CLISH_PLUGIN_SYM(name) int name(void *clish_context, const char *script, char **out)

#define CLISH_DEFAULT_SYM "clish_script@clish"

typedef CLISH_PLUGIN_SYM(clish_plugin_fn_t);
typedef CLISH_PLUGIN_INIT_FUNC(clish_plugin_init_t);

/* Symbol */

int clish_sym_compare(const void *first, const void *second);
clish_sym_t *clish_sym_new(const char *name, clish_plugin_fn_t *func);
void clish_sym_free(clish_sym_t *instance);
void clish_sym__set_func(clish_sym_t *instance, clish_plugin_fn_t *func);
clish_plugin_fn_t *clish_sym__get_func(clish_sym_t *instance);
void clish_sym__set_name(clish_sym_t *instance, const char *name);
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
