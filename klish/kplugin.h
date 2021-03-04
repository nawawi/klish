/** @file kplugin.h
 *
 * @brief Klish scheme's "plugin" entry
 */

#ifndef _klish_kplugin_h
#define _klish_kplugin_h

#include <faux/error.h>

// Plugin API version
#define KPLUGIN_MAJOR 1
#define KPLUGIN_MINOR 1


typedef struct kplugin_s kplugin_t;

typedef struct iplugin_s {
	char *name;
	char *id;
	char *file;
	char *global;
	char *script;
} iplugin_t;


typedef enum {
	KPLUGIN_ERROR_OK,
	KPLUGIN_ERROR_INTERNAL,
	KPLUGIN_ERROR_ALLOC,
	KPLUGIN_ERROR_ATTR_NAME,
	KPLUGIN_ERROR_ATTR_ID,
	KPLUGIN_ERROR_ATTR_FILE,
	KPLUGIN_ERROR_ATTR_GLOBAL,
	KPLUGIN_ERROR_SCRIPT,
} kplugin_error_e;


C_DECL_BEGIN

// iplugin_t
char *iplugin_to_text(const iplugin_t *iplugin, int level);

// kplugin_t
void kplugin_free(kplugin_t *plugin);
bool_t kplugin_parse(kplugin_t *plugin, const iplugin_t *info, kplugin_error_e *error);
kplugin_t *kplugin_new(const iplugin_t *info, kplugin_error_e *error);
const char *kplugin_strerror(kplugin_error_e error);

const char *kplugin_name(const kplugin_t *plugin);
bool_t kplugin_set_name(kplugin_t *plugin, const char *name);
const char *kplugin_id(const kplugin_t *plugin);
bool_t kplugin_set_id(kplugin_t *plugin, const char *id);
const char *kplugin_file(const kplugin_t *plugin);
bool_t kplugin_set_file(kplugin_t *plugin, const char *file);
bool_t kplugin_global(const kplugin_t *plugin);
bool_t kplugin_set_global(kplugin_t *plugin, bool_t global);
const char *kplugin_script(const kplugin_t *plugin);
bool_t kplugin_set_script(kplugin_t *plugin, const char *script);

kplugin_t *kplugin_from_iplugin(iplugin_t *iplugin, faux_error_t *error_stack);

C_DECL_END

#endif // _klish_kplugin_h
