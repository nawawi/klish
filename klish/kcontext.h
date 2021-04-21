/** @file kcontext.h
 *
 * @brief Klish context to pass to plugin's functions
 */

#ifndef _klish_kcontext_h
#define _klish_kcontext_h

#include <klish/kcontext_base.h>
#include <klish/kscheme.h>


C_DECL_BEGIN

int kcontext_retcode(const kcontext_t *context);
bool_t kcontext_set_retcode(kcontext_t *context, int retcode);
kplugin_t *kcontext_plugin(const kcontext_t *context);
bool_t kcontext_set_plugin(kcontext_t *context, kplugin_t *plugin);
ksym_t *kcontext_sym(const kcontext_t *context);
bool_t kcontext_set_sym(kcontext_t *context, ksym_t *sym);
kaction_t *kcontext_action(const kcontext_t *context);
bool_t kcontext_set_action(kcontext_t *context, kaction_t *action);
kcommand_t *kcontext_command(const kcontext_t *context);
bool_t kcontext_set_command(kcontext_t *context, kcommand_t *command);

C_DECL_END

#endif // _klish_kcontext_h
