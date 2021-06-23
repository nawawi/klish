/** @file kpargv.h
 *
 * @brief Parsed ARGuments Vector
 */

#ifndef _klish_kpargv_h
#define _klish_kpargv_h

#include <faux/list.h>
#include <klish/kentry.h>

typedef struct kpargv_s kpargv_t;
typedef struct kparg_s kparg_t;

typedef faux_list_node_t kpargv_pargs_node_t;


C_DECL_BEGIN

// Parg

kparg_t *kparg_new(kentry_t *entry, const char *value);
void kparg_free(kparg_t *parg);

kentry_t *kparg_entry(const kparg_t *parg);
bool_t kparg_set_value(kparg_t *parg, const char *value);
const char *kparg_value(const kparg_t *parg);

// Pargv

kpargv_t *kpargv_new();
void kpargv_free(kpargv_t *pargv);

faux_list_t *kpargv_pargs(const kpargv_t *pargv);
ssize_t kpargv_pargs_len(const kpargv_t *pargv);
bool_t kpargv_pargs_is_empty(const kpargv_t *pargv);
bool_t kpargv_pargs_add(kpargv_t *pargv, kparg_t *parg);
kparg_t *kpargv_pargs_last(const kpargv_t *pargv);

C_DECL_END

#endif // _klish_kpargv_h
