/** @file view.h
 *
 * @brief Klish scheme's "view" entry
 */

#ifndef _klish_kview_h
#define _klish_kview_h

#include <faux/faux.h>
#include <faux/list.h>
#include <klish/kcommand.h>


typedef struct kview_s kview_t;

typedef faux_list_node_t kview_commands_node_t;


C_DECL_BEGIN

kview_t *kview_new(const char *name);
void kview_free(kview_t *view);

const char *kview_name(const kview_t *view);

// commands
bool_t kview_add_command(kview_t *view, kcommand_t *command);
kcommand_t *kview_find_command(const kview_t *view, const char *name);
ssize_t kview_commands_len(const kview_t *view);
kview_commands_node_t *kview_commands_iter(const kview_t *view);
kcommand_t *kview_commands_each(kview_commands_node_t **iter);

C_DECL_END

#endif // _klish_kview_h
