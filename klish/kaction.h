/** @file action.h
 *
 * @brief Klish scheme's "action" entry
 */

#ifndef _klish_kaction_h
#define _klish_kaction_h

#include <faux/error.h>

typedef struct kaction_s kaction_t;

typedef struct iaction_s {
	char *sym;
	char *lock;
	char *interrupt;
	char *interactive;
	char *exec_on;
	char *update_retcode;
} iaction_t;


typedef enum {
	KACTION_ERROR_OK,
	KACTION_ERROR_INTERNAL,
	KACTION_ERROR_ALLOC,
	KACTION_ERROR_ATTR_SYM,
	KACTION_ERROR_ATTR_LOCK,
	KACTION_ERROR_ATTR_INTERRUPT,
	KACTION_ERROR_ATTR_INTERACTIVE,
	KACTION_ERROR_ATTR_EXEC_ON,
	KACTION_ERROR_ATTR_UPDATE_RETCODE,
} kaction_error_e;


typedef enum {
	KACTION_COND_FAIL,
	KACTION_COND_SUCCESS,
	KACTION_COND_ALWAYS
} kaction_cond_e;


C_DECL_BEGIN

kaction_t *kaction_new(const iaction_t *info, kaction_error_e *error);
void kaction_free(kaction_t *action);
const char *kaction_strerror(kaction_error_e error);
bool_t kaction_parse(kaction_t *action, const iaction_t *info, kaction_error_e *error);

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

kaction_t *kaction_from_iaction(iaction_t *iaction, faux_error_t *error_stack);

C_DECL_END

#endif // _klish_kaction_h
