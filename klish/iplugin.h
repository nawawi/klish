/** @file iplugin.h
 *
 * @brief Klish scheme's "plugin" entry
 */

#ifndef _klish_iplugin_h
#define _klish_iplugin_h

#include <klish/kplugin.h>

typedef struct iplugin_s {
	char *name;
	char *id;
	char *file;
	char *conf;
} iplugin_t;


C_DECL_BEGIN

bool_t iplugin_parse(const iplugin_t *info, kplugin_t *plugin,
	faux_error_t *error);
kplugin_t *iplugin_load(iplugin_t *iplugin, faux_error_t *error);
char *iplugin_deploy(const kplugin_t *kplugin, int level);

C_DECL_END

#endif // _klish_iplugin_h
