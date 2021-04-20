/** @file kaction.h
 *
 * @brief Klish scheme's "action" entry
 */

#ifndef _klish_kaction_h
#define _klish_kaction_h

#include <faux/error.h>
#include <klish/ksym.h>


typedef struct kaction_s kaction_t;

typedef enum {
	KACTION_COND_NONE,
	KACTION_COND_FAIL,
	KACTION_COND_SUCCESS,
	KACTION_COND_ALWAYS
} kaction_cond_e;


C_DECL_BEGIN

kaction_t *kaction_new(void);
void kaction_free(kaction_t *action);

const char *kaction_sym_ref(const kaction_t *action);
bool_t kaction_set_sym_ref(kaction_t *action, const char *sym_ref);
const char *kaction_lock(const kaction_t *action);
bool_t kaction_set_lock(kaction_t *action, const char *lock);
bool_t kaction_interrupt(const kaction_t *action);
bool_t kaction_set_interrupt(kaction_t *action, bool_t interrupt);
bool_t kaction_interactive(const kaction_t *action);
bool_t kaction_set_interactive(kaction_t *action, bool_t interactive);
kaction_cond_e kaction_exec_on(const kaction_t *action);
bool_t kaction_set_exec_on(kaction_t *action, kaction_cond_e exec_on);
bool_t kaction_update_retcode(const kaction_t *action);
bool_t kaction_set_update_retcode(kaction_t *action, bool_t update_retcode);
const char *kaction_script(const kaction_t *action);
bool_t kaction_set_script(kaction_t *action, const char *script);
ksym_t *kaction_sym(const kaction_t *action);
bool_t kaction_set_sym(kaction_t *action, ksym_t *sym);

C_DECL_END

#endif // _klish_kaction_h
