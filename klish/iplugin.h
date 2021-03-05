/** @file iplugin.h
 *
 * @brief Klish scheme's "plugin" entry
 */

#ifndef _klish_iplugin_h
#define _klish_iplugin_h

typedef struct iplugin_s {
	char *name;
	char *id;
	char *file;
	char *global;
	char *conf;
} iplugin_t;


C_DECL_BEGIN

char *iplugin_to_text(const iplugin_t *iplugin, int level);

C_DECL_END

#endif // _klish_iplugin_h
