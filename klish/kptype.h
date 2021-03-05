/** @file kptype.h
 *
 * @brief Klish scheme's "ptype" entry
 */

#ifndef _klish_kptype_h
#define _klish_kptype_h

#include <faux/error.h>
#include <klish/iptype.h>
#include <klish/kaction.h>

typedef struct kptype_s kptype_t;

typedef enum {
	KPTYPE_ERROR_OK,
	KPTYPE_ERROR_INTERNAL,
	KPTYPE_ERROR_ALLOC,
	KPTYPE_ERROR_ATTR_NAME,
	KPTYPE_ERROR_ATTR_HELP,
} kptype_error_e;


C_DECL_BEGIN

void kptype_free(kptype_t *ptype);
bool_t kptype_parse(kptype_t *ptype, const iptype_t *info, kptype_error_e *error);
kptype_t *kptype_new(const iptype_t *info, kptype_error_e *error);
const char *kptype_strerror(kptype_error_e error);

const char *kptype_name(const kptype_t *ptype);
bool_t kptype_set_name(kptype_t *ptype, const char *name);
const char *kptype_help(const kptype_t *ptype);
bool_t kptype_set_help(kptype_t *ptype, const char *help);

bool_t kptype_nested_from_iptype(kptype_t *kptype, iptype_t *iptype,
	faux_error_t *error_stack);
kptype_t *kptype_from_iptype(iptype_t *iptype, faux_error_t *error_stack);

C_DECL_END

#endif // _klish_kptype_h
