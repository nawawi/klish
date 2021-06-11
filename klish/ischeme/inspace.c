#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <faux/list.h>
#include <faux/error.h>
#include <klish/khelper.h>
#include <klish/knspace.h>
#include <klish/inspace.h>

#define TAG "NSPACE"


bool_t inspace_parse(const inspace_t *info, knspace_t *nspace, faux_error_t *error)
{
	bool_t retcode = BOOL_TRUE;

	if (!info)
		return BOOL_FALSE;
	if (!nspace)
		return BOOL_FALSE;

	// Prefix
	if (!faux_str_is_empty(info->prefix)) {
		if (!knspace_set_prefix(nspace, info->prefix)) {
			faux_error_add(error, TAG": Illegal 'prefix' attribute");
			retcode = BOOL_FALSE;
		}
	}

	return retcode;
}


bool_t inspace_parse_nested(const inspace_t *inspace, knspace_t *knspace,
	faux_error_t *error)
{
	bool_t retval = BOOL_TRUE;

	if (!knspace || !inspace) {
		faux_error_add(error, TAG": Internal error");
		return BOOL_FALSE;
	}

	// For now it's empty. NSPACE has no nested elements.

	if (!retval)
		faux_error_sprintf(error, TAG" \"%s\": Illegal nested elements",
			knspace_view_ref(knspace));

	return retval;
}


knspace_t *inspace_load(const inspace_t *inspace, faux_error_t *error)
{
	knspace_t *knspace = NULL;

	if (!inspace)
		return NULL;

	// Ref [mandatory]
	if (faux_str_is_empty(inspace->ref)) {
		faux_error_add(error, TAG": Empty 'ref' attribute");
		return NULL;
	}

	knspace = knspace_new(inspace->ref);
	if (!knspace) {
		faux_error_sprintf(error, TAG": \"%s\": Can't create object",
			inspace->ref);
		return NULL;
	}

	if (!inspace_parse(inspace, knspace, error)) {
		knspace_free(knspace);
		return NULL;
	}

	// Parse nested elements
	if (!inspace_parse_nested(inspace, knspace, error)) {
		knspace_free(knspace);
		return NULL;
	}

	return knspace;
}


char *inspace_deploy(const knspace_t *knspace, int level)
{
	char *str = NULL;
	char *tmp = NULL;

	if (!knspace)
		return NULL;

	tmp = faux_str_sprintf("%*cnspace {\n", level, ' ');
	faux_str_cat(&str, tmp);
	faux_str_free(tmp);

	attr2ctext(&str, "ref", knspace_view_ref(knspace), level + 1);
	attr2ctext(&str, "prefix", knspace_prefix(knspace), level + 1);

	tmp = faux_str_sprintf("%*c},\n\n", level, ' ');
	faux_str_cat(&str, tmp);
	faux_str_free(tmp);

	return str;
}
