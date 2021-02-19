/** @file kparam.h
 *
 * @brief Klish scheme's "param" entry
 */

#ifndef _klish_kparam_h
#define _klish_kparam_h

typedef struct kparam_s kparam_t;

typedef struct iparam_s {
	char *name;
	char *help;
	char *ptype;
} iparam_t;


C_DECL_BEGIN

kparam_t *kparam_new(iparam_t info);
kparam_t *kparam_new_static(iparam_t info);
void kparam_free(kparam_t *param);

const char *kparam_name(const kparam_t *param);
const char *kparam_help(const kparam_t *param);
const char *kparam_ptype_str(const kparam_t *param);

C_DECL_END

#endif // _klish_kparam_h
