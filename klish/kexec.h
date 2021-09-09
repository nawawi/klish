/** @file kexec.h
 *
 * @brief Klish exec
 */

#ifndef _klish_kexec_h
#define _klish_kexec_h

#include <faux/list.h>
#include <klish/kcontext.h>

typedef struct kexec_s kexec_t;

typedef faux_list_node_t kexec_contexts_node_t;


C_DECL_BEGIN

kexec_t *kexec_new(void);
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
// Return code
bool_t kexec_done(const kexec_t *exec);
bool_t kexec_retcode(const kexec_t *exec, int *status);
// CONTEXTs
bool_t kexec_add_contexts(kexec_t *exec, kcontext_t *context);
ssize_t kexec_contexts_len(const kexec_t *exec);
bool_t kexec_contexts_is_empty(const kexec_t *exec);
kexec_contexts_node_t *kexec_contexts_iter(const kexec_t *exec);
kcontext_t *kexec_contexts_each(kexec_contexts_node_t **iter);

bool_t kexec_exec(kexec_t *exec);


C_DECL_END

#endif // _klish_kexec_h
