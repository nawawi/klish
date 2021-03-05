/** @file view.h
 *
 * @brief Klish scheme's "view" entry
 */

#ifndef _klish_iview_h
#define _klish_iview_h

#include <klish/icommand.h>

typedef struct iview_s {
	char *name;
	icommand_t * (*commands)[];
} iview_t;

C_DECL_BEGIN

char *iview_to_text(const iview_t *iview, int level);

C_DECL_END

#endif // _klish_iview_h
