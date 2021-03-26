/** @file kcontext.h
 *
 * @brief Klish context to pass to plugin's functions
 */

#ifndef _klish_kcontext_h
#define _klish_kcontext_h

#include <faux/error.h>

#include <klish/kscheme.h>


typedef struct kcontext_s kcontext_t;

typedef enum {
	KCONTEXT_NONE,
	KCONTEXT_PLUGIN_INIT,
	KCONTEXT_PLUGIN_FINI,
	KCONTEXT_PLUGIN_ACTION
} kcontext_type_e;


C_DECL_BEGIN

kcontext_t *kcontext_new(kcontext_type_e type);
void kcontext_free(kcontext_t *context);

kcontext_type_e kcontext_type(const kcontext_t *context);
bool_t kcontext_set_type(kcontext_t *context, kcontext_type_e type);
int kcontext_retcode(const kcontext_t *context);
bool_t kcontext_set_retcode(kcontext_t *context, int retcode);
const kplugin_t *kcontext_plugin(const kcontext_t *context);
bool_t kcontext_set_plugin(kcontext_t *context, const kplugin_t *plugin);
const ksym_t *kcontext_sym(const kcontext_t *context);
bool_t kcontext_set_sym(kcontext_t *context, const ksym_t *sym);
const kaction_t *kcontext_action(const kcontext_t *context);
bool_t kcontext_set_action(kcontext_t *context, const kaction_t *action);
const kcommand_t *kcontext_command(const kcontext_t *context);
bool_t kcontext_set_command(kcontext_t *context, const kcommand_t *command);

C_DECL_END

#endif // _klish_kcontext_h
