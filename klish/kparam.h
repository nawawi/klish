/** @file kparam.h
 *
 * @brief Klish scheme's "param" entry
 */

#ifndef _klish_kparam_h
#define _klish_kparam_h

#include <faux/error.h>


typedef struct kparam_s kparam_t;


C_DECL_BEGIN

kparam_t *kparam_new(const char *name);
void kparam_free(kparam_t *param);

const char *kparam_name(const kparam_t *param);
const char *kparam_help(const kparam_t *param);
bool_t kparam_set_help(kparam_t *param, const char *help);
const char *kparam_ptype_ref(const kparam_t *param);
bool_t kparam_set_ptype_ref(kparam_t *param, const char *ptype_ref);

bool_t kparam_add_param(kparam_t *param, kparam_t *nested_param);
kparam_t *kparam_find_param(const kparam_t *param, const char *name);

C_DECL_END

#endif // _klish_kparam_h
