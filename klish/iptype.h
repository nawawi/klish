/** @file iptype.h
 *
 * @brief Klish scheme's "ptype" entry
 */

#ifndef _klish_iptype_h
#define _klish_iptype_h

#include <faux/error.h>
#include <klish/iaction.h>
#include <klish/kptype.h>

typedef struct iptype_s {
	char *name;
	char *help;
	iaction_t * (*actions)[];
} iptype_t;

C_DECL_BEGIN

bool_t iptype_parse(const iptype_t *info, kptype_t *ptype, faux_error_t *error);
bool_t iptype_parse_nested(const iptype_t *iptype, kptype_t *kptype,
	faux_error_t *error_stack);
kptype_t *iptype_load(const iptype_t *iptype, faux_error_t *error_stack);
char *iptype_deploy(const kptype_t *kptype, int level);

C_DECL_END

#endif // _klish_iptype_h
