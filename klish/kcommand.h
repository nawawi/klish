/** @file kcommand.h
 *
 * @brief Klish scheme's "command" entry
 */

#ifndef _klish_kcommand_h
#define _klish_kcommand_h

#include <klish/kparam.h>
#include <klish/kaction.h>

typedef struct kcommand_s kcommand_t;

typedef struct icommand_s {
	char *name;
	char *help;
	iparam_t * (*params)[];
	iaction_t * (*actions)[];
} icommand_t;

C_DECL_BEGIN

kcommand_t *kcommand_new(icommand_t info);
kcommand_t *kcommand_new_static(icommand_t info);
void kcommand_free(kcommand_t *command);

const char *kcommand_name(const kcommand_t *command);
const char *kcommand_help(const kcommand_t *command);

bool_t kcommand_add_param(kcommand_t *command, kparam_t *param);

C_DECL_END

#endif // _klish_kcommand_h
