/** @file nspace.h
 *
 * @brief Klish scheme's "nspace" entry
 */

#ifndef _klish_inspace_h
#define _klish_inspace_h

#include <faux/error.h>
#include <klish/icommand.h>
#include <klish/knspace.h>

typedef struct inspace_s {
	char *ref;
	char *prefix;
} inspace_t;

C_DECL_BEGIN

bool_t inspace_parse(const inspace_t *info, knspace_t *nspace, faux_error_t *error);
bool_t inspace_parse_nested(const inspace_t *inspace, knspace_t *knspace,
	faux_error_t *error);
knspace_t *inspace_load(const inspace_t *inspace, faux_error_t *error);
char *inspace_deploy(const knspace_t *knspace, int level);

C_DECL_END

#endif // _klish_inspace_h
