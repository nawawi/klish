/** @file kptype.h
 *
 * @brief Klish scheme's "ptype" entry
 */

#ifndef _klish_kptype_h
#define _klish_kptype_h

#include <klish/kaction.h>

typedef struct kptype_s kptype_t;

typedef struct iptype_s {
	char *name;
	char *help;
	iaction_t * (*actions)[];
} iptype_t;


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

C_DECL_END

#endif // _klish_kptype_h
