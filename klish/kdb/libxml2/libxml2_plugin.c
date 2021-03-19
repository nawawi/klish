#include <errno.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include <faux/faux.h>
#include <faux/str.h>
#include <klish/kxml.h>
#include <klish/kscheme.h>


kscheme_t *kxml_load_scheme(const char *xml_path, faux_error_t *error);


bool_t kdb_libxml2_init(void)
{
	kscheme_t *scheme = NULL;

	scheme = kxml_load_scheme(NULL, NULL);
scheme = scheme;
	return BOOL_TRUE;
}
