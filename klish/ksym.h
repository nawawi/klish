/** @file ksym.h
 *
 * @brief Klish symbol
 */

#ifndef _klish_ksym_h
#define _klish_ksym_h


typedef struct ksym_s ksym_t;


C_DECL_BEGIN

// ksym_t
ksym_t *ksym_new(const char *name);
void ksym_free(ksym_t *sym);

const char *ksym_name(const ksym_t *sym);
const void *ksym_fn(const ksym_t *sym);
bool_t ksym_set_fn(ksym_t *sym, const void *fn);

C_DECL_END

#endif // _klish_ksym_h
