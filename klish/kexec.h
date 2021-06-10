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

size_t kexec_len(const kexec_t *exec);
size_t kexec_is_empty(const kexec_t *exec);
bool_t kexec_add(kexec_t *exec, kcontext_t *context);

// STDIN
int kexec_stdin(const kexec_t *exec);
bool_t kexec_set_stdin(kexec_t *exec, int stdin);
// STDOUT
int kexec_stdout(const kexec_t *exec);
bool_t kexec_set_stdout(kexec_t *exec, int stdout);
// STDERR
int kexec_stderr(const kexec_t *exec);
bool_t kexec_set_stderr(kexec_t *exec, int stderr);

C_DECL_END

#endif // _klish_kexec_h
