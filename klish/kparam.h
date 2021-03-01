/** @file kparam.h
 *
 * @brief Klish scheme's "param" entry
 */

#ifndef _klish_kparam_h
#define _klish_kparam_h

typedef struct kparam_s kparam_t;

typedef struct iparam_s iparam_t;
struct iparam_s {
	char *name;
	char *help;
	char *ptype;
	iparam_t * (*params)[]; // Nested PARAMs
};


typedef enum {
	KPARAM_ERROR_OK,
	KPARAM_ERROR_INTERNAL,
	KPARAM_ERROR_ALLOC,
	KPARAM_ERROR_ATTR_NAME,
	KPARAM_ERROR_ATTR_HELP,
	KPARAM_ERROR_ATTR_PTYPE,
} kparam_error_e;


C_DECL_BEGIN

kparam_t *kparam_new(const iparam_t *info, kparam_error_e *error);
void kparam_free(kparam_t *param);
const char *kparam_strerror(kparam_error_e error);
bool_t kparam_parse(kparam_t *param, const iparam_t *info, kparam_error_e *error);

const char *kparam_name(const kparam_t *param);
bool_t kparam_set_name(kparam_t *param, const char *name);
const char *kparam_help(const kparam_t *param);
bool_t kparam_set_help(kparam_t *param, const char *help);
const char *kparam_ptype_ref(const kparam_t *param);
bool_t kparam_set_ptype_ref(kparam_t *param, const char *ptype_ref);

bool_t kparam_add_param(kparam_t *param, kparam_t *nested_param);
kparam_t *kparam_find_param(const kparam_t *param, const char *name);

bool_t kparam_nested_from_iparam(kparam_t *kparam, iparam_t *iparam,
	faux_error_t *error_stack);
kparam_t *kparam_from_iparam(iparam_t *iparam, faux_error_t *error_stack);

C_DECL_END

#endif // _klish_kparam_h
