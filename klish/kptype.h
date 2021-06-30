/** @file kptype.h
 *
 * @brief Klish scheme's "ptype" entry
 */

#ifndef _klish_kptype_h
#define _klish_kptype_h

#include <faux/list.h>
#include <klish/kaction.h>

typedef struct kptype_s kptype_t;

typedef faux_list_node_t kptype_actions_node_t;

C_DECL_BEGIN

kptype_t *kptype_new(const char *name);
void kptype_free(kptype_t *ptype);

const char *kptype_name(const kptype_t *ptype);
const char *kptype_help(const kptype_t *ptype);
bool_t kptype_set_help(kptype_t *ptype, const char *help);

// ACTIONs
faux_list_t *kptype_actions(const kptype_t *ptype);
bool_t kptype_add_actions(kptype_t *plugin, kaction_t *action);
ssize_t kptype_actions_len(const kptype_t *plugin);
kptype_actions_node_t *kptype_actions_iter(const kptype_t *plugin);
kaction_t *kptype_actions_each(kptype_actions_node_t **iter);

C_DECL_END

#endif // _klish_kptype_h
