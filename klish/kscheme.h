/** @file kscheme.h
 *
 * @brief Klish Scheme
 */

#ifndef _klish_kscheme_h
#define _klish_kscheme_h

#include <faux/error.h>

#include <klish/kptype.h>
#include <klish/kplugin.h>
#include <klish/kaction.h>
#include <klish/kparam.h>
#include <klish/kcommand.h>
#include <klish/kview.h>


#define VIEW_LIST .views = &(iview_t * []) {
#define END_VIEW_LIST NULL }
#define VIEW &(iview_t)

#define PTYPE_LIST .ptypes = &(iptype_t * []) {
#define END_PTYPE_LIST NULL }
#define PTYPE &(iptype_t)

#define COMMAND_LIST .commands = &(icommand_t * []) {
#define END_COMMAND_LIST NULL }
#define COMMAND &(icommand_t)

#define PARAM_LIST .params = &(iparam_t * []) {
#define END_PARAM_LIST NULL }
#define PARAM &(iparam_t)

#define ACTION_LIST .actions = &(iaction_t * []) {
#define END_ACTION_LIST NULL }
#define ACTION &(iaction_t)

#define PLUGIN_LIST .plugins = &(iplugin_t * []) {
#define END_PLUGIN_LIST NULL }
#define PLUGIN &(iplugin_t)


typedef struct kscheme_s kscheme_t;

typedef struct ischeme_s {
	char *name;
	iplugin_t * (*plugins)[];
	iptype_t * (*ptypes)[];
	iview_t * (*views)[];
} ischeme_t;


typedef enum {
	KSCHEME_ERROR_OK,
	KSCHEME_ERROR_INTERNAL,
	KSCHEME_ERROR_ALLOC,
} kscheme_error_e;


C_DECL_BEGIN

// ischeme_t
char *ischeme_to_text(const ischeme_t *ischeme, int level);

// kscheme_t
kscheme_t *kscheme_new(kscheme_error_e *error);
void kscheme_free(kscheme_t *scheme);
const char *kscheme_strerror(kscheme_error_e error);

bool_t kscheme_add_view(kscheme_t *scheme, kview_t *view);
kview_t *kscheme_find_view(const kscheme_t *scheme, const char *name);
bool_t kscheme_add_ptype(kscheme_t *scheme, kptype_t *ptype);
kptype_t *kscheme_find_ptype(const kscheme_t *scheme, const char *name);
bool_t kscheme_add_plugin(kscheme_t *scheme, kplugin_t *plugin);
kplugin_t *kscheme_find_plugin(const kscheme_t *scheme, const char *name);

bool_t kscheme_nested_from_ischeme(kscheme_t *kscheme, ischeme_t *ischeme,
	faux_error_t *error_stack);
kscheme_t *kscheme_from_ischeme(ischeme_t *ischeme, faux_error_t *error_stack);

C_DECL_END

#endif // _klish_kscheme_h
