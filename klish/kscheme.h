/** @file kscheme.h
 *
 * @brief Klish Scheme
 */

#ifndef _klish_kscheme_h
#define _klish_kscheme_h

#include <faux/list.h>
#include <klish/kplugin.h>
#include <klish/kptype.h>
#include <klish/kview.h>
#include <klish/kcontext_base.h>


typedef struct kscheme_s kscheme_t;

typedef faux_list_node_t kscheme_views_node_t;
typedef faux_list_node_t kscheme_ptypes_node_t;
typedef faux_list_node_t kscheme_plugins_node_t;


C_DECL_BEGIN

kscheme_t *kscheme_new(void);
void kscheme_free(kscheme_t *scheme);

// views
bool_t kscheme_add_view(kscheme_t *scheme, kview_t *view);
kview_t *kscheme_find_view(const kscheme_t *scheme, const char *name);
ssize_t kscheme_views_len(const kscheme_t *scheme);
kscheme_views_node_t *kscheme_views_iter(const kscheme_t *scheme);
kview_t *kscheme_views_each(kscheme_views_node_t **iter);
// ptypes
bool_t kscheme_add_ptype(kscheme_t *scheme, kptype_t *ptype);
kptype_t *kscheme_find_ptype(const kscheme_t *scheme, const char *name);
ssize_t kscheme_ptypes_len(const kscheme_t *scheme);
kscheme_ptypes_node_t *kscheme_ptypes_iter(const kscheme_t *scheme);
kptype_t *kscheme_ptypes_each(kscheme_ptypes_node_t **iter);
// plugins
bool_t kscheme_add_plugin(kscheme_t *scheme, kplugin_t *plugin);
kplugin_t *kscheme_find_plugin(const kscheme_t *scheme, const char *name);
ssize_t kscheme_plugins_len(const kscheme_t *scheme);
kscheme_plugins_node_t *kscheme_plugins_iter(const kscheme_t *scheme);
kplugin_t *kscheme_plugins_each(kscheme_plugins_node_t **iter);

bool_t kscheme_prepare(kscheme_t *scheme, kcontext_t *context, faux_error_t *error);
bool_t kscheme_fini(kscheme_t *scheme, kcontext_t *context, faux_error_t *error);


C_DECL_END

#endif // _klish_kscheme_h
