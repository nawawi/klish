/** @file ihotkey.h
 *
 * @brief Klish scheme's "hotkey" entry
 */

#ifndef _klish_ihotkey_h
#define _klish_ihotkey_h

#include <faux/error.h>
#include <klish/khotkey.h>

typedef struct ihotkey_s {
	char *key;
	char *cmd;
} ihotkey_t;


C_DECL_BEGIN

//bool_t ihotkey_parse(const ihotkey_t *info, khotkey_t *hotkey,
//	faux_error_t *error);
khotkey_t *ihotkey_load(const ihotkey_t *ihotkey, faux_error_t *error);
char *ihotkey_deploy(const khotkey_t *khotkey, int level);

C_DECL_END

#endif // _klish_ihotkey_h
