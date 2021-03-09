/** @file kscheme.h
 *
 * @brief Klish Scheme
 */

#ifndef _klish_kscheme_h
#define _klish_kscheme_h

#include <faux/error.h>

#include <klish/kplugin.h>
#include <klish/kptype.h>
#include <klish/kview.h>


typedef struct kscheme_s kscheme_t;


C_DECL_BEGIN

kscheme_t *kscheme_new(void);
void kscheme_free(kscheme_t *scheme);

bool_t kscheme_add_view(kscheme_t *scheme, kview_t *view);
kview_t *kscheme_find_view(const kscheme_t *scheme, const char *name);
bool_t kscheme_add_ptype(kscheme_t *scheme, kptype_t *ptype);
kptype_t *kscheme_find_ptype(const kscheme_t *scheme, const char *name);
bool_t kscheme_add_plugin(kscheme_t *scheme, kplugin_t *plugin);
kplugin_t *kscheme_find_plugin(const kscheme_t *scheme, const char *name);

C_DECL_END

#endif // _klish_kscheme_h
