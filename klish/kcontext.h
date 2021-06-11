/** @file kcontext.h
 *
 * @brief Klish context to pass to plugin's functions
 */

#ifndef _klish_kcontext_h
#define _klish_kcontext_h

#include <klish/kcontext_base.h>
#include <klish/kscheme.h>


C_DECL_BEGIN

// Type
kcontext_type_e kcontext_type(const kcontext_t *context);
FAUX_HIDDEN bool_t kcontext_set_type(kcontext_t *context, kcontext_type_e type);
// RetCode
int kcontext_retcode(const kcontext_t *context);
FAUX_HIDDEN bool_t kcontext_set_retcode(kcontext_t *context, int retcode);
// Plugin
kplugin_t *kcontext_plugin(const kcontext_t *context);
FAUX_HIDDEN bool_t kcontext_set_plugin(kcontext_t *context, kplugin_t *plugin);
// Sym
ksym_t *kcontext_sym(const kcontext_t *context);
FAUX_HIDDEN bool_t kcontext_set_sym(kcontext_t *context, ksym_t *sym);
// Action
kaction_t *kcontext_action(const kcontext_t *context);
FAUX_HIDDEN bool_t kcontext_set_action(kcontext_t *context, kaction_t *action);
// Command
kcommand_t *kcontext_command(const kcontext_t *context);
FAUX_HIDDEN bool_t kcontext_set_command(kcontext_t *context, kcommand_t *command);
// STDIN
int kcontext_stdin(const kcontext_t *context);
FAUX_HIDDEN bool_t kcontext_set_stdin(kcontext_t *context, int stdin);
// STDOUT
int kcontext_stdout(const kcontext_t *context);
FAUX_HIDDEN bool_t kcontext_set_stdout(kcontext_t *context, int stdout);
// STDERR
int kcontext_stderr(const kcontext_t *context);
FAUX_HIDDEN bool_t kcontext_set_stderr(kcontext_t *context, int stderr);
// PID
pid_t kcontext_pid(const kcontext_t *context);
FAUX_HIDDEN bool_t kcontext_set_pid(kcontext_t *context, pid_t pid);

C_DECL_END

#endif // _klish_kcontext_h
