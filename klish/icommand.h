/** @file icommand.h
 *
 * @brief Klish scheme's "command" entry
 */

#ifndef _klish_icommand_h
#define _klish_icommand_h

#include <klish/iparam.h>
#include <klish/iaction.h>

typedef struct icommand_s {
	char *name;
	char *help;
	iparam_t * (*params)[];
	iaction_t * (*actions)[];
} icommand_t;

C_DECL_BEGIN

char *icommand_to_text(const icommand_t *icommand, int level);

C_DECL_END

#endif // _klish_icommand_h
