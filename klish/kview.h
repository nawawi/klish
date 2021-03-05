/** @file view.h
 *
 * @brief Klish scheme's "view" entry
 */

#ifndef _klish_kview_h
#define _klish_kview_h

#include <faux/faux.h>
#include <faux/error.h>
#include <klish/iview.h>
#include <klish/kcommand.h>

typedef struct kview_s kview_t;

typedef enum {
	KVIEW_ERROR_OK,
	KVIEW_ERROR_INTERNAL,
	KVIEW_ERROR_ALLOC,
	KVIEW_ERROR_ATTR_NAME,
} kview_error_e;


C_DECL_BEGIN

// kview_t
kview_t *kview_new(const iview_t *info, kview_error_e *error);
void kview_free(kview_t *view);
bool_t kview_parse(kview_t *view, const iview_t *info, kview_error_e *error);
const char *kview_strerror(kview_error_e error);

const char *kview_name(const kview_t *view);
bool_t kview_set_name(kview_t *view, const char *name);

bool_t kview_add_command(kview_t *view, kcommand_t *command);

bool_t kview_nested_from_iview(kview_t *kview, iview_t *iview,
	faux_error_t *error_stack);
kview_t *kview_from_iview(iview_t *iview, faux_error_t *error_stack);

C_DECL_END

#endif // _klish_kview_h
