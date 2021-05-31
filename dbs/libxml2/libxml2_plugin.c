#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include <faux/faux.h>
#include <faux/str.h>
#include <faux/error.h>
#include <klish/kxml.h>
#include <klish/kscheme.h>
#include <klish/kdb.h>


uint8_t kdb_libxml2_major = KDB_MAJOR;
uint8_t kdb_libxml2_minor = KDB_MINOR;


kscheme_t *kdb_libxml2_load_scheme(kdb_t *db)
{
	faux_ini_t *ini = NULL;
	faux_error_t *error = NULL;
	const char *xml_path = NULL;

	assert(db);
	if (!db)
		return BOOL_FALSE;

	// Get configuration info from kdb object
	ini = kdb_ini(db);
	if (ini)
		xml_path = faux_ini_find(ini, "XMLPath");
	error = kdb_error(db);

	return kxml_load_scheme(xml_path, error);
}
