/** @file kscheme.h
 *
 * @brief Klish Scheme
 */

#ifndef _klish_kscheme_h
#define _klish_kscheme_h

#include <faux/list.h>
#include <klish/kplugin.h>
#include <klish/kentry.h>
#include <klish/kcontext_base.h>
#include <klish/kudata.h>


typedef struct kscheme_s kscheme_t;

typedef faux_list_node_t kscheme_plugins_node_t;
typedef faux_list_node_t kscheme_entrys_node_t;


C_DECL_BEGIN

kscheme_t *kscheme_new(void);
void kscheme_free(kscheme_t *scheme);

bool_t kscheme_prepare(kscheme_t *scheme, kcontext_t *context, faux_error_t *error);
bool_t kscheme_fini(kscheme_t *scheme, kcontext_t *context, faux_error_t *error);

// PLUGINs
faux_list_t *kscheme_plugins(const kscheme_t *scheme);
bool_t kscheme_add_plugins(kscheme_t *scheme, kplugin_t *plugin);
kplugin_t *kscheme_find_plugin(const kscheme_t *scheme, const char *name);
ssize_t kscheme_plugins_len(const kscheme_t *scheme);
kscheme_plugins_node_t *kscheme_plugins_iter(const kscheme_t *scheme);
kplugin_t *kscheme_plugins_each(kscheme_plugins_node_t **iter);

// ENTRYs
kentry_t *kscheme_find_entry_by_path(const kscheme_t *scheme, const char *name);
faux_list_t *kscheme_entrys(const kscheme_t *scheme);
bool_t kscheme_add_entrys(kscheme_t *scheme, kentry_t *entry);
kentry_t *kscheme_find_entry(const kscheme_t *scheme, const char *name);
ssize_t kscheme_entrys_len(const kscheme_t *scheme);
kscheme_entrys_node_t *kscheme_entrys_iter(const kscheme_t *scheme);
kentry_t *kscheme_entrys_each(kscheme_entrys_node_t **iter);

// User data store
bool_t kscheme_named_udata_new(kscheme_t *scheme,
	const char *name, void *data, kudata_data_free_fn free_fn);
void *kscheme_named_udata(kscheme_t *scheme, const char *name);


C_DECL_END

#endif // _klish_kscheme_h
