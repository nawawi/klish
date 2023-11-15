/** @file kcontext.h
 *
 * @brief Klish context to pass to plugin's functions
 */

#ifndef _klish_kcontext_h
#define _klish_kcontext_h

#include <faux/list.h>
#include <klish/kcontext_base.h>
#include <klish/kpargv.h>
#include <klish/kscheme.h>
#include <klish/ksession.h>
#include <klish/kaction.h>
#include <klish/kudata.h>


C_DECL_BEGIN

// Type
kcontext_type_e kcontext_type(const kcontext_t *context);
FAUX_HIDDEN bool_t kcontext_set_type(kcontext_t *context, kcontext_type_e type);

// Scheme
kscheme_t *kcontext_scheme(const kcontext_t *context);
bool_t kcontext_set_scheme(kcontext_t *context, kscheme_t *scheme);

// RetCode
int kcontext_retcode(const kcontext_t *context);
FAUX_HIDDEN bool_t kcontext_set_retcode(kcontext_t *context, int retcode);

// Plugin
kplugin_t *kcontext_plugin(const kcontext_t *context);
FAUX_HIDDEN bool_t kcontext_set_plugin(kcontext_t *context, kplugin_t *plugin);

// Sym
ksym_t *kcontext_sym(const kcontext_t *context);
FAUX_HIDDEN bool_t kcontext_set_sym(kcontext_t *context, ksym_t *sym);

// Pargv object
kpargv_t *kcontext_pargv(const kcontext_t *context);
FAUX_HIDDEN bool_t kcontext_set_pargv(kcontext_t *context, kpargv_t *pargv);

// Parent pargv object
const kpargv_t *kcontext_parent_pargv(const kcontext_t *context);
FAUX_HIDDEN bool_t kcontext_set_parent_pargv(kcontext_t *context,
	const kpargv_t *parent_pargv);

// Parent context object
const kcontext_t *kcontext_parent_context(const kcontext_t *context);
FAUX_HIDDEN bool_t kcontext_set_parent_context(kcontext_t *context,
	const kcontext_t *parent_context);

// Action iterator
faux_list_node_t *kcontext_action_iter(const kcontext_t *context);
FAUX_HIDDEN bool_t kcontext_set_action_iter(kcontext_t *context, faux_list_node_t *action_iter);

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

// Session
ksession_t *kcontext_session(const kcontext_t *context);
FAUX_HIDDEN bool_t kcontext_set_session(kcontext_t *context, ksession_t *session);

// Done
bool_t kcontext_done(const kcontext_t *context);
FAUX_HIDDEN bool_t kcontext_set_done(kcontext_t *context, bool_t done);

// Wrappers
kparg_t *kcontext_candidate_parg(const kcontext_t *context);
const kentry_t *kcontext_candidate_entry(const kcontext_t *context);
const char *kcontext_candidate_value(const kcontext_t *context);
const kaction_t *kcontext_action(const kcontext_t *context);
const char *kcontext_script(const kcontext_t *context);
bool_t kcontext_named_udata_new(kcontext_t *context,
	const char *name, void *data, kudata_data_free_fn free_fn);
void *kcontext_named_udata(const kcontext_t *context, const char *name);
void *kcontext_udata(const kcontext_t *context);
const kentry_t *kcontext_command(const kcontext_t *context);


C_DECL_END

#endif // _klish_kcontext_h
