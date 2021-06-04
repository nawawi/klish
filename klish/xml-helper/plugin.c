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


bool_t kxml_plugin_init(kdb_t *db)
{
	db = db; // Happy compiler

	return kxml_doc_start();
}


bool_t kxml_plugin_fini(kdb_t *db)
{
	db = db; // Happy compiler

	return kxml_doc_stop();
}


bool_t kxml_plugin_load_scheme(kdb_t *db, kscheme_t *scheme)
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

	return kxml_load_scheme(scheme, xml_path, error);
}
