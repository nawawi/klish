/** @file iaction.h
 *
 * @brief Klish scheme's "action" entry
 */

#ifndef _klish_iaction_h
#define _klish_iaction_h

#include <faux/error.h>


typedef struct iaction_s {
	char *sym;
	char *lock;
	char *interrupt;
	char *interactive;
	char *exec_on;
	char *update_retcode;
	char *script;
} iaction_t;


C_DECL_BEGIN

char *iaction_to_text(const iaction_t *iaction, int level);

C_DECL_END

#endif // _klish_iaction_h
