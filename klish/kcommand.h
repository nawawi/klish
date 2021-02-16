/** @file kcommand.h
 *
 * @brief Klish scheme's "command" entry
 */

#ifndef _klish_kcommand_h
#define _klish_kcommand_h

typedef struct kcommand_s kcommand_t;

typedef struct kcommand_info_s {
	char *name;
	char *help;
} kcommand_info_t;


C_DECL_BEGIN

kcommand_t *kcommand_new(kcommand_info_t info);
kcommand_t *kcommand_new_static(kcommand_info_t info);
void kcommand_free(kcommand_t *command);

const char *kcommand_name(const kcommand_t *command);
const char *kcommand_help(const kcommand_t *command);

C_DECL_END

#endif // _klish_kcommand_h
