/** @file khotkey.h
 *
 * @brief Klish scheme's "hotkey" entry
 */

#ifndef _klish_khotkey_h
#define _klish_khotkey_h

#include <faux/error.h>


typedef struct khotkey_s khotkey_t;


C_DECL_BEGIN

khotkey_t *khotkey_new(const char *key, const char *cmd);
void khotkey_free(khotkey_t *hotkey);

const char *khotkey_key(const khotkey_t *hotkey);
const char *khotkey_cmd(const khotkey_t *hotkey);

C_DECL_END

#endif // _klish_khotkey_h
