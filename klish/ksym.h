/** @file ksym.h
 *
 * @brief Klish symbol
 */

#ifndef _klish_ksym_h
#define _klish_ksym_h

#include <faux/error.h>

typedef struct ksym_s ksym_t;

typedef enum {
	KSYM_ERROR_OK,
	KSYM_ERROR_INTERNAL,
	KSYM_ERROR_ALLOC,
} ksym_error_e;


C_DECL_BEGIN

// ksym_t
ksym_t *ksym_new(ksym_error_e *error);
void ksym_free(ksym_t *sym);
const char *ksym_strerror(ksym_error_e error);

const char *ksym_name(const ksym_t *sym);
bool_t ksym_set_name(ksym_t *sym, const char *name);
const void *ksym_fn(const ksym_t *sym);
bool_t ksym_set_fn(ksym_t *sym, const void *fn);

C_DECL_END

#endif // _klish_ksym_h
