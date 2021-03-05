/** @file kcommand.h
 *
 * @brief Klish scheme's "command" entry
 */

#ifndef _klish_kcommand_h
#define _klish_kcommand_h

#include <klish/icommand.h>
#include <klish/kparam.h>
#include <klish/kaction.h>

typedef struct kcommand_s kcommand_t;

typedef enum {
	KCOMMAND_ERROR_OK,
	KCOMMAND_ERROR_INTERNAL,
	KCOMMAND_ERROR_ALLOC,
	KCOMMAND_ERROR_ATTR_NAME,
	KCOMMAND_ERROR_ATTR_HELP,
} kcommand_error_e;

C_DECL_BEGIN

kcommand_t *kcommand_new(const icommand_t *info, kcommand_error_e *error);
void kcommand_free(kcommand_t *command);
const char *kcommand_strerror(kcommand_error_e error);
bool_t kcommand_parse(kcommand_t *command, const icommand_t *info,
	kcommand_error_e *error);

const char *kcommand_name(const kcommand_t *command);
bool_t kcommand_set_name(kcommand_t *command, const char *name);
const char *kcommand_help(const kcommand_t *command);
bool_t kcommand_set_help(kcommand_t *command, const char *help);

bool_t kcommand_add_param(kcommand_t *command, kparam_t *param);
kparam_t *kcommand_find_param(const kcommand_t *command, const char *name);
bool_t kcommand_add_action(kcommand_t *command, kaction_t *action);

bool_t kcommand_nested_from_icommand(kcommand_t *kcommand, icommand_t *icommand,
	faux_error_t *error_stack);
kcommand_t *kcommand_from_icommand(icommand_t *icommand, faux_error_t *error_stack);

C_DECL_END

#endif // _klish_kcommand_h
