/** @file iptype.h
 *
 * @brief Klish scheme's "ptype" entry
 */

#ifndef _klish_iptype_h
#define _klish_iptype_h

#include <klish/iaction.h>

typedef struct iptype_s {
	char *name;
	char *help;
	iaction_t * (*actions)[];
} iptype_t;

C_DECL_BEGIN

char *iptype_to_text(const iptype_t *iptype, int level);

C_DECL_END

#endif // _klish_iptype_h
