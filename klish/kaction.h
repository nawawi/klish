/** @file action.h
 *
 * @brief Klish scheme's "action" entry
 */

#ifndef _klish_kaction_h
#define _klish_kaction_h

#include <klish/kcommand.h>

typedef struct kaction_s kaction_t;

typedef struct kaction_info_s {
	char *sym;
} kaction_info_t;


C_DECL_BEGIN

kaction_t *kaction_new(kaction_info_t info);
kaction_t *kaction_new_static(kaction_info_t info);
void kaction_free(kaction_t *action);

const char *kaction_name(const kaction_t *action);

bool_t kaction_add_command(kaction_t *action, kcommand_t *command);

C_DECL_END

#endif // _klish_kaction_h
