/** @file ksym.h
 *
 * @brief Klish symbol
 */

#ifndef _klish_ksym_h
#define _klish_ksym_h


typedef struct ksym_s ksym_t;

// Callback function prototype
typedef struct kcontext_s kcontext_t; // Redeclaration to don't include kcontext.h
typedef int (*ksym_fn)(kcontext_t *context);


C_DECL_BEGIN

// ksym_t
ksym_t *ksym_new(const char *name);
void ksym_free(ksym_t *sym);

const char *ksym_name(const ksym_t *sym);
const ksym_fn *ksym_function(const ksym_t *sym);
bool_t ksym_set_function(ksym_t *sym, const ksym_fn *fn);

C_DECL_END

#endif // _klish_ksym_h
