/** @file kcommand.h
 *
 * @brief Klish scheme's "command" entry
 */

#ifndef _klish_kcommand_h
#define _klish_kcommand_h

#include <faux/list.h>
#include <klish/kparam.h>
#include <klish/kaction.h>


typedef struct kcommand_s kcommand_t;

typedef faux_list_node_t kcommand_params_node_t;
typedef faux_list_node_t kcommand_actions_node_t;

C_DECL_BEGIN

kcommand_t *kcommand_new(const char *name);
void kcommand_free(kcommand_t *command);

const char *kcommand_name(const kcommand_t *command);
const char *kcommand_help(const kcommand_t *command);
bool_t kcommand_set_help(kcommand_t *command, const char *help);

// params
bool_t kcommand_add_param(kcommand_t *command, kparam_t *param);
kparam_t *kcommand_find_param(const kcommand_t *command, const char *name);
ssize_t kcommand_params_len(const kcommand_t *command);
kcommand_params_node_t *kcommand_params_iter(const kcommand_t *command);
kparam_t *kcommand_params_each(kcommand_params_node_t **iter);
// actions
bool_t kcommand_add_action(kcommand_t *command, kaction_t *action);
ssize_t kcommand_actions_len(const kcommand_t *command);
kcommand_actions_node_t *kcommand_actions_iter(const kcommand_t *command);
kaction_t *kcommand_actions_each(kcommand_actions_node_t **iter);

C_DECL_END

#endif // _klish_kcommand_h
