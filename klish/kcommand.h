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


C_DECL_BEGIN

kcommand_t *kcommand_new(const char *name);
void kcommand_free(kcommand_t *command);

const char *kcommand_name(const kcommand_t *command);
const char *kcommand_help(const kcommand_t *command);
bool_t kcommand_set_help(kcommand_t *command, const char *help);

bool_t kcommand_add_param(kcommand_t *command, kparam_t *param);
kparam_t *kcommand_find_param(const kcommand_t *command, const char *name);
ssize_t kcommand_params_len(const kcommand_t *command);
bool_t kcommand_add_action(kcommand_t *command, kaction_t *action);
ssize_t kcommand_actions_len(const kcommand_t *command);

bool_t kcommand_parse(kcommand_t *command, const icommand_t *info,
	faux_error_t *error);
bool_t kcommand_nested_from_icommand(kcommand_t *kcommand, icommand_t *icommand,
	faux_error_t *error);
kcommand_t *kcommand_from_icommand(icommand_t *icommand, faux_error_t *error);

C_DECL_END

#endif // _klish_kcommand_h
