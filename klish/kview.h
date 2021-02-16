/** @file view.h
 *
 * @brief Klish scheme's "view" entry
 */

#ifndef _klish_kview_h
#define _klish_kview_h

#include <klish/kcommand.h>

typedef struct kview_s kview_t;

typedef struct kview_info_s {
	char *name;
} kview_info_t;


C_DECL_BEGIN

kview_t *kview_new(kview_info_t info);
kview_t *kview_new_static(kview_info_t info);
void kview_free(kview_t *view);

const char *kview_name(const kview_t *view);

bool_t kview_add_command(kview_t *view, kcommand_t *command);

C_DECL_END

#endif // _klish_kview_h
