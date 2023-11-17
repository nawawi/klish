/** @file kdb.h
 *
 * @brief Klish db
 */

#ifndef _klish_kdb_h
#define _klish_kdb_h

#include <stdint.h>

#include <faux/ini.h>
#include <faux/error.h>
#include <klish/kscheme.h>

// Current API version
#define KDB_MAJOR 1
#define KDB_MINOR 0

// Shared object filename template. Insert db ID or db "name" field
// instead "%s". Consider db ID as an "internal native name". The "name"
// field can differ from ID and it's just used within scheme to refer db.
// Consider it as alias of ID.
#define KDB_SONAME_FMT "libklish-db-%s.so"

// db's API version symbols
// One byte (uint8_t) for major and one byte for minor numbers
#define KDB_MAJOR_FMT "kdb_%s_major"
#define KDB_MINOR_FMT "kdb_%s_minor"

// db's init and fini functions
// Init and fini functions can be not defined
#define KDB_INIT_FMT "kdb_%s_init"
#define KDB_FINI_FMT "kdb_%s_fini"

// db's load and deploy functions
// One of these function must be non-NULL else plugin has no usefull functions
#define KDB_LOAD_FMT "kdb_%s_load_scheme"
#define KDB_DEPLOY_FMT "kdb_%s_deploy_scheme"


typedef struct kdb_s kdb_t;

// DB plugin's entry points
typedef bool_t (*kdb_init_fn)(kdb_t *db);
typedef bool_t (*kdb_fini_fn)(kdb_t *db);
typedef bool_t (*kdb_load_fn)(kdb_t *db, kscheme_t *scheme);
typedef bool_t (*kdb_deploy_fn)(kdb_t *db, const kscheme_t *scheme);


C_DECL_BEGIN

kdb_t *kdb_new(const char *name, const char *file);
void kdb_free(kdb_t *db);

const char *kdb_name(const kdb_t *db);
const char *kdb_file(const kdb_t *db);
faux_ini_t *kdb_ini(const kdb_t *db);
bool_t kdb_set_ini(kdb_t *db, faux_ini_t *ini);
uint8_t kdb_major(const kdb_t *db);
// static bool_t kdb_set_major(kdb_t *db, uint8_t major);
uint8_t kdb_minor(const kdb_t *db);
// static bool_t kdb_set_minor(kdb_t *db, uint8_t minor);
void *kdb_udata(const kdb_t *db);
bool_t kdb_set_udata(kdb_t *db, void *udata);
faux_error_t *kdb_error(const kdb_t *db);
bool_t kdb_set_error(kdb_t *db, faux_error_t *error);
bool_t kdb_load_plugin(kdb_t *db);
bool_t kdb_init(kdb_t *db);
bool_t kdb_fini(kdb_t *db);
bool_t kdb_load_scheme(kdb_t *db, kscheme_t *scheme);
bool_t kdb_deploy_scheme(kdb_t *db, const kscheme_t *scheme);
bool_t kdb_has_init_fn(const kdb_t *db);
bool_t kdb_has_fini_fn(const kdb_t *db);
bool_t kdb_has_load_fn(const kdb_t *db);
bool_t kdb_has_deploy_fn(const kdb_t *db);

C_DECL_END

#endif // _klish_kdb_h
