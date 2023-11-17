/** @file kplugin.h
 *
 * @brief Klish scheme's "plugin" entry
 */

#ifndef _klish_kplugin_h
#define _klish_kplugin_h

#include <stdint.h>

#include <faux/list.h>
#include <klish/ksym.h>
#include <klish/kcontext_base.h>

// Current API version
// Major and minor version numbers is uint8_t
#define KPLUGIN_MAJOR 1
#define KPLUGIN_MINOR 0

// Shared object filename template. Insert plugin ID or plugin "name" field
// instead "%s". Consider plugin ID as an "internal native name". The "name"
// field can differ from ID and it's just used within scheme to refer plugin.
// Consider it as alias of ID.
#define KPLUGIN_SONAME_FMT "libklish-plugin-%s.so"

// Plugin's API version symbols
#define KPLUGIN_MAJOR_FMT "kplugin_%s_major"
#define KPLUGIN_MINOR_FMT "kplugin_%s_minor"

// Plugin's init and fini functions
#define KPLUGIN_INIT_FMT "kplugin_%s_init"
#define KPLUGIN_FINI_FMT "kplugin_%s_fini"

// Plugin's options
#define KPLUGIN_OPT_GLOBAL_FMT "kplugin_%s_opt_global"


typedef struct kplugin_s kplugin_t;

typedef faux_list_node_t kplugin_syms_node_t;


C_DECL_BEGIN

kplugin_t *kplugin_new(const char *name);
void kplugin_free(kplugin_t *plugin);

const char *kplugin_name(const kplugin_t *plugin);
const char *kplugin_id(const kplugin_t *plugin);
bool_t kplugin_set_id(kplugin_t *plugin, const char *id);
const char *kplugin_file(const kplugin_t *plugin);
bool_t kplugin_set_file(kplugin_t *plugin, const char *file);
const char *kplugin_conf(const kplugin_t *plugin);
bool_t kplugin_set_conf(kplugin_t *plugin, const char *conf);
uint8_t kplugin_major(const kplugin_t *plugin);
bool_t kplugin_set_major(kplugin_t *plugin, uint8_t major);
uint8_t kplugin_minor(const kplugin_t *plugin);
bool_t kplugin_set_minor(kplugin_t *plugin, uint8_t minor);
void *kplugin_udata(const kplugin_t *plugin);
bool_t kplugin_set_udata(kplugin_t *plugin, void *udata);
bool_t kplugin_load(kplugin_t *plugin);
ksym_fn kplugin_init_session_fn(const kplugin_t *plugin);
bool_t kplugin_set_init_session_fn(kplugin_t *plugin, ksym_fn fn);
ksym_fn kplugin_fini_session_fn(const kplugin_t *plugin);
bool_t kplugin_set_fini_session_fn(kplugin_t *plugin, ksym_fn fn);
int kplugin_init_session(kplugin_t *plugin, kcontext_t *context);
int kplugin_fini_session(kplugin_t *plugin, kcontext_t *context);
int kplugin_init(kplugin_t *plugin, kcontext_t *context);
int kplugin_fini(kplugin_t *plugin, kcontext_t *context);

// SYMs
faux_list_t *kplugin_syms(const kplugin_t *plugin);
bool_t kplugin_add_syms(kplugin_t *plugin, ksym_t *sym);
ksym_t *kplugin_find_sym(const kplugin_t *plugin, const char *name);
ssize_t kplugin_syms_len(const kplugin_t *plugin);
kplugin_syms_node_t *kplugin_syms_iter(const kplugin_t *plugin);
ksym_t *kplugin_syms_each(kplugin_syms_node_t **iter);

C_DECL_END

#endif // _klish_kplugin_h
