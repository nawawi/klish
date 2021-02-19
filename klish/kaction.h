/** @file action.h
 *
 * @brief Klish scheme's "action" entry
 */

#ifndef _klish_kaction_h
#define _klish_kaction_h

#include <klish/kcommand.h>

typedef struct kaction_s kaction_t;

typedef struct iaction_s {
	char *sym;
	char *exec_on;
	char *update_retcode;
} iaction_t;


typedef enum {
	KACTION_ERROR_OK,
	KACTION_ERROR_MALLOC,
	KACTION_ERROR_LIST
} kaction_error_e;

typedef enum {
	KACTION_COND_FAIL,
	KACTION_COND_SUCCESS,
	KACTION_COND_ALWAYS
} kaction_cond_e;


C_DECL_BEGIN

kaction_t *kaction_new(iaction_t info);
kaction_t *kaction_new_static(iaction_t info);
void kaction_free(kaction_t *action);

const char *kaction_name(const kaction_t *action);

bool_t kaction_add_command(kaction_t *action, kcommand_t *command);

C_DECL_END

#endif // _klish_kaction_h
