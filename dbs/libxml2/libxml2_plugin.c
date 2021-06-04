#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include <faux/faux.h>
#include <faux/str.h>
#include <faux/error.h>
#include <klish/kxml.h>
#include <klish/kscheme.h>
#include <klish/kdb.h>


uint8_t kdb_libxml2_major = KDB_MAJOR;
uint8_t kdb_libxml2_minor = KDB_MINOR;


bool_t kdb_libxml2_init(kdb_t *db)
{
	return kxml_plugin_init(db);
}

bool_t kdb_libxml2_fini(kdb_t *db)
{
	return kxml_plugin_fini(db);
}


bool_t kdb_libxml2_load_scheme(kdb_t *db, kscheme_t *scheme)
{
	return kxml_plugin_load_scheme(db, scheme);
}
