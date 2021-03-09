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

bool_t iaction_parse(const iaction_t *info, kaction_t *action,
	faux_error_t *error);
kaction_t *iaction_load(const iaction_t *iaction, faux_error_t *error);
char *iaction_deploy(const kaction_t *kaction, int level);

C_DECL_END

#endif // _klish_iaction_h
