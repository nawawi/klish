#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <faux/list.h>
#include <faux/error.h>
#include <klish/khelper.h>
#include <klish/kcommand.h>
#include <klish/kview.h>

#define TAG "VIEW"


bool_t kview_parse(kview_t *view, const iview_t *info, faux_error_t *error)
{
	bool_t retcode = BOOL_TRUE;

	view = view;
	info = info;
	error = error;

	return retcode;
}


bool_t kview_nested_from_iview(kview_t *kview, iview_t *iview,
	faux_error_t *error)
{
	bool_t retval = BOOL_TRUE;

	if (!kview || !iview) {
		faux_error_add(error, TAG": Internal error");
		return BOOL_FALSE;
	}

	// COMMAND list
	if (iview->commands) {
		icommand_t **p_icommand = NULL;
		for (p_icommand = *iview->commands; *p_icommand; p_icommand++) {
			kcommand_t *kcommand = NULL;
			icommand_t *icommand = *p_icommand;
			kcommand = kcommand_from_icommand(icommand, error);
			if (!kcommand) {
				retval = BOOL_FALSE;
				continue;
			}
			if (!kview_add_command(kview, kcommand)) {
				// Search for COMMAND duplicates
				if (kview_find_command(kview,
					kcommand_name(kcommand))) {
					faux_error_sprintf(error,
						TAG": Can't add duplicate COMMAND "
						"\"%s\"", kcommand_name(kcommand));
				} else {
					faux_error_sprintf(error,
						TAG": Can't add COMMAND \"%s\"",
						kcommand_name(kcommand));
				}
				kcommand_free(kcommand);
				retval = BOOL_FALSE;
				continue;
			}
		}
	}

	if (!retval)
		faux_error_sprintf(error, TAG" \"%s\": Illegal nested elements",
			kview_name(kview));

	return retval;
}


kview_t *kview_from_iview(iview_t *iview, faux_error_t *error)
{
	kview_t *kview = NULL;

	// Name [mandatory]
	if (faux_str_is_empty(iview->name)) {
		faux_error_add(error, TAG": Empty 'name' attribute");
		return NULL;
	}

	kview = kview_new(iview->name);
	if (!kview) {
		faux_error_sprintf(error, TAG": \"%s\": Can't create object",
			iview->name);
		return NULL;
	}

	if (!kview_parse(kview, iview, error)) {
		kview_free(kview);
		return NULL;
	}

	// Parse nested elements
	if (!kview_nested_from_iview(kview, iview, error)) {
		kview_free(kview);
		return NULL;
	}

	return kview;
}


char *iview_to_text(const iview_t *iview, int level)
{
	char *str = NULL;
	char *tmp = NULL;

	tmp = faux_str_sprintf("%*cVIEW {\n", level, ' ');
	faux_str_cat(&str, tmp);
	faux_str_free(tmp);

	attr2ctext(&str, "name", iview->name, level + 1);

	// COMMAND list
	if (iview->commands) {
		icommand_t **p_icommand = NULL;

		tmp = faux_str_sprintf("\n%*cCOMMAND_LIST\n\n", level + 1, ' ');
		faux_str_cat(&str, tmp);
		faux_str_free(tmp);

		for (p_icommand = *iview->commands; *p_icommand; p_icommand++) {
			icommand_t *icommand = *p_icommand;

			tmp = icommand_to_text(icommand, level + 2);
			faux_str_cat(&str, tmp);
			faux_str_free(tmp);
		}

		tmp = faux_str_sprintf("%*cEND_COMMAND_LIST,\n", level + 1, ' ');
		faux_str_cat(&str, tmp);
		faux_str_free(tmp);
	}

	tmp = faux_str_sprintf("%*c},\n\n", level, ' ');
	faux_str_cat(&str, tmp);
	faux_str_free(tmp);

	return str;
}
