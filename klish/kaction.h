/** @file kaction.h
 *
 * @brief Klish scheme's "action" entry
 */

#ifndef _klish_kaction_h
#define _klish_kaction_h

#include <faux/error.h>
#include <klish/ksym.h>
#include <klish/kplugin.h>


typedef struct kaction_s kaction_t;

typedef enum {
	KACTION_COND_NONE,
	KACTION_COND_FAIL,
	KACTION_COND_SUCCESS,
	KACTION_COND_ALWAYS,
	KACTION_COND_NEVER,
} kaction_cond_e;

typedef enum {
	KACTION_IO_NONE,
	KACTION_IO_FALSE,
	KACTION_IO_TRUE,
	KACTION_IO_TTY,
	KACTION_IO_MAX,
} kaction_io_e;


C_DECL_BEGIN

kaction_t *kaction_new(void);
void kaction_free(kaction_t *action);

const char *kaction_sym_ref(const kaction_t *action);
bool_t kaction_set_sym_ref(kaction_t *action, const char *sym_ref);

const char *kaction_lock(const kaction_t *action);
bool_t kaction_set_lock(kaction_t *action, const char *lock);

bool_t kaction_interrupt(const kaction_t *action);
bool_t kaction_set_interrupt(kaction_t *action, bool_t interrupt);

kaction_io_e kaction_in(const kaction_t *action);
bool_t kaction_set_in(kaction_t *action, kaction_io_e in);

kaction_io_e kaction_out(const kaction_t *action);
bool_t kaction_set_out(kaction_t *action, kaction_io_e out);

kaction_cond_e kaction_exec_on(const kaction_t *action);
bool_t kaction_set_exec_on(kaction_t *action, kaction_cond_e exec_on);
bool_t kaction_meet_exec_conditions(const kaction_t *action, int current_retcode);

bool_t kaction_update_retcode(const kaction_t *action);
bool_t kaction_set_update_retcode(kaction_t *action, bool_t update_retcode);

const char *kaction_script(const kaction_t *action);
bool_t kaction_set_script(kaction_t *action, const char *script);

ksym_t *kaction_sym(const kaction_t *action);
bool_t kaction_set_sym(kaction_t *action, ksym_t *sym);

kplugin_t *kaction_plugin(const kaction_t *action);
bool_t kaction_set_plugin(kaction_t *action, kplugin_t *plugin);

tri_t kaction_permanent(const kaction_t *action);
bool_t kaction_set_permanent(kaction_t *action, tri_t permanent);
bool_t kaction_is_permanent(const kaction_t *action);

tri_t kaction_sync(const kaction_t *action);
bool_t kaction_set_sync(kaction_t *action, tri_t sync);
bool_t kaction_is_sync(const kaction_t *action);

C_DECL_END

#endif // _klish_kaction_h
