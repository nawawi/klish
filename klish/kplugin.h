/** @file kplugin.h
 *
 * @brief Klish scheme's "plugin" entry
 */

#ifndef _klish_kplugin_h
#define _klish_kplugin_h

#include <stdint.h>
#include <faux/error.h>

#include <klish/iplugin.h>
#include <klish/ksym.h>

// Current API version
#define KPLUGIN_MAJOR 1
#define KPLUGIN_MINOR 0

// Shared object filename template. Insert plugin ID or plugin "name" field
// instead "%s". Consider plugin ID as an "internal native name". The "name"
// field can differ from ID and it's just used within scheme to refer plugin.
// Consider it as alias of ID.
#define KPLUGIN_SONAME_FMT "kplugin_%s.so"

// Plugin's API version symbols
#define KPLUGIN_MAJOR_FMT "kplugin_%s_major"
#define KPLUGIN_MINOR_FMT "kplugin_%s_minor"

// Plugin's init and fini functions
#define KPLUGIN_INIT_FMT "kplugin_%s_init"
#define KPLUGIN_FINI_FMT "kplugin_%s_fini"


typedef struct kplugin_s kplugin_t;

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
void *kplugin_udata(const kplugin_t *plugin);
bool_t kplugin_set_udata(kplugin_t *plugin, void *udata);

bool_t kplugin_add_sym(kplugin_t *plugin, ksym_t *sym);
ksym_t *kplugin_find_sym(const kplugin_t *plugin, const char *name);

kplugin_t *kplugin_from_iplugin(iplugin_t *iplugin, faux_error_t *error_stack);

C_DECL_END

#endif // _klish_kplugin_h
