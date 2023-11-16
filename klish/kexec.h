/** @file kexec.h
 *
 * @brief Klish exec
 */

#ifndef _klish_kexec_h
#define _klish_kexec_h

#include <faux/list.h>
#include <faux/buf.h>
#include <klish/kcontext.h>
#include <klish/ksession.h>

typedef struct kexec_s kexec_t;

typedef faux_list_node_t kexec_contexts_node_t;


C_DECL_BEGIN

kexec_t *kexec_new(ksession_t *session, kcontext_type_e type);
void kexec_free(kexec_t *exec);

// Dry-run
bool_t kexec_dry_run(const kexec_t *exec);
bool_t kexec_set_dry_run(kexec_t *exec, bool_t dry_run);
// STDIN
int kexec_stdin(const kexec_t *exec);
bool_t kexec_set_stdin(kexec_t *exec, int stdin);
// STDOUT
int kexec_stdout(const kexec_t *exec);
bool_t kexec_set_stdout(kexec_t *exec, int stdout);
// STDERR
int kexec_stderr(const kexec_t *exec);
bool_t kexec_set_stderr(kexec_t *exec, int stderr);
// BUFIN
faux_buf_t *kexec_bufin(const kexec_t *exec);
bool_t kexec_set_bufin(kexec_t *exec, faux_buf_t *bufin);
// BUFOUT
faux_buf_t *kexec_bufout(const kexec_t *exec);
bool_t kexec_set_bufout(kexec_t *exec, faux_buf_t *bufout);
// BUFERR
faux_buf_t *kexec_buferr(const kexec_t *exec);
bool_t kexec_set_buferr(kexec_t *exec, faux_buf_t *buferr);
// Return code
bool_t kexec_done(const kexec_t *exec);
bool_t kexec_retcode(const kexec_t *exec, int *status);
// Saved path
kpath_t *kexec_saved_path(const kexec_t *exec);
// Line
const char *kexec_line(const kexec_t *exec);
bool_t kexec_set_line(kexec_t *exec, const char *line);

// CONTEXTs
bool_t kexec_add_contexts(kexec_t *exec, kcontext_t *context);
ssize_t kexec_contexts_len(const kexec_t *exec);
bool_t kexec_contexts_is_empty(const kexec_t *exec);
kexec_contexts_node_t *kexec_contexts_iter(const kexec_t *exec);
kcontext_t *kexec_contexts_each(kexec_contexts_node_t **iter);

bool_t kexec_continue_command_execution(kexec_t *exec, pid_t pid, int wstatus);
bool_t kexec_exec(kexec_t *exec);
bool_t kexec_need_stdin(const kexec_t *exec);
bool_t kexec_interactive(const kexec_t *exec);
bool_t kexec_set_winsize(kexec_t *exec);
const kaction_t *kexec_current_action(const kexec_t *exec);

C_DECL_END

#endif // _klish_kexec_h
