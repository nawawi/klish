/** @file kplugin.h
 *
 * @brief Klish scheme's "plugin" entry
 */

#ifndef _klish_kplugin_h
#define _klish_kplugin_h

#include <stdint.h>
#include <faux/error.h>

#include <klish/ksym.h>

// Plugin API version
#define KPLUGIN_MAJOR 1
#define KPLUGIN_MINOR 0


typedef struct kplugin_s kplugin_t;

typedef struct iplugin_s {
	char *name;
	char *id;
	char *file;
	char *global;
	char *conf;
} iplugin_t;


typedef enum {
	KPLUGIN_ERROR_OK,
	KPLUGIN_ERROR_INTERNAL,
	KPLUGIN_ERROR_ALLOC,
	KPLUGIN_ERROR_ATTR_NAME,
	KPLUGIN_ERROR_ATTR_ID,
	KPLUGIN_ERROR_ATTR_FILE,
	KPLUGIN_ERROR_ATTR_GLOBAL,
	KPLUGIN_ERROR_ATTR_CONF,
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
const char *kplugin_conf(const kplugin_t *plugin);
bool_t kplugin_set_conf(kplugin_t *plugin, const char *conf);
uint8_t kplugin_major(const kplugin_t *plugin);
bool_t kplugin_set_major(kplugin_t *plugin, uint8_t major);
uint8_t kplugin_minor(const kplugin_t *plugin);
bool_t kplugin_set_minor(kplugin_t *plugin, uint8_t minor);

bool_t kplugin_add_sym(kplugin_t *plugin, ksym_t *sym);
ksym_t *kplugin_find_sym(const kplugin_t *plugin, const char *name);

kplugin_t *kplugin_from_iplugin(iplugin_t *iplugin, faux_error_t *error_stack);

C_DECL_END

#endif // _klish_kplugin_h
