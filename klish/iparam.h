/** @file iparam.h
 *
 * @brief Klish scheme's "param" entry
 */

#ifndef _klish_iparam_h
#define _klish_iparam_h

#include <faux/error.h>
#include <klish/kparam.h>

typedef struct iparam_s iparam_t;

struct iparam_s {
	char *name;
	char *help;
	char *ptype;
	iparam_t * (*params)[]; // Nested PARAMs
};

C_DECL_BEGIN

bool_t iparam_parse(const iparam_t *info, kparam_t *param, faux_error_t *error);
bool_t iparam_parse_nested(const iparam_t *iparam, kparam_t *kparam,
	faux_error_t *error);
kparam_t *iparam_load(const iparam_t *iparam, faux_error_t *error);
char *iparam_deploy(const kparam_t *kparam, int level);

C_DECL_END

#endif // _klish_iparam_h
