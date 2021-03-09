/** @file icommand.h
 *
 * @brief Klish scheme's "command" entry
 */

#ifndef _klish_icommand_h
#define _klish_icommand_h

#include <klish/iparam.h>
#include <klish/iaction.h>
#include <klish/kcommand.h>

typedef struct icommand_s {
	char *name;
	char *help;
	iparam_t * (*params)[];
	iaction_t * (*actions)[];
} icommand_t;

C_DECL_BEGIN

bool_t icommand_parse(const icommand_t *info, kcommand_t *command,
	faux_error_t *error);
bool_t icommand_parse_nested(const icommand_t *icommand, kcommand_t *kcommand,
	faux_error_t *error);
kcommand_t *icommand_load(icommand_t *icommand, faux_error_t *error);
char *icommand_deploy(const kcommand_t *kcommand, int level);

C_DECL_END

#endif // _klish_icommand_h
