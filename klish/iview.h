/** @file view.h
 *
 * @brief Klish scheme's "view" entry
 */

#ifndef _klish_iview_h
#define _klish_iview_h

#include <faux/error.h>
#include <klish/icommand.h>

typedef struct iview_s {
	char *name;
	icommand_t * (*commands)[];
} iview_t;

C_DECL_BEGIN

bool_t iview_parse(const iview_t *info, kview_t *view, faux_error_t *error);
bool_t iview_parse_nested(const iview_t *iview, kview_t *kview,
	faux_error_t *error);
kview_t *iview_load(const iview_t *iview, faux_error_t *error);
char *iview_deploy(const kview_t *kview, int level);

C_DECL_END

#endif // _klish_iview_h
