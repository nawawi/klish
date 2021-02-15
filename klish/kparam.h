/** @file kparam.h
 *
 * @brief Klish scheme's "param" entry
 */

#ifndef _klish_kparam_h
#define _klish_kparam_h

typedef struct kparam_s kparam_t;

typedef struct kparam_info_s {
	char *name;
	char *help;
	char *ptype;
} kparam_info_t;


C_DECL_BEGIN

kparam_t *kparam_new(kparam_info_t info);
kparam_t *kparam_new_static(kparam_info_t info);
void kparam_free(kparam_t *param);

C_DECL_END

#endif // _klish_kparam_h
