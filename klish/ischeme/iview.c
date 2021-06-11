#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <faux/list.h>
#include <faux/error.h>
#include <klish/khelper.h>
#include <klish/kcommand.h>
#include <klish/knspace.h>
#include <klish/kview.h>
#include <klish/iview.h>

#define TAG "VIEW"


bool_t iview_parse(const iview_t *info, kview_t *view, faux_error_t *error)
{
	bool_t retcode = BOOL_TRUE;

	if (!info)
		return BOOL_FALSE;
	if (!view)
		return BOOL_FALSE;

	view = view;
	info = info;
	error = error;

	return retcode;
}


bool_t iview_parse_nested(const iview_t *iview, kview_t *kview,
	faux_error_t *error)
{
	bool_t retval = BOOL_TRUE;

	if (!kview || !iview) {
		faux_error_add(error, TAG": Internal error");
		return BOOL_FALSE;
	}

	// NSPACE list
	if (iview->nspaces) {
		inspace_t **p_inspace = NULL;
		for (p_inspace = *iview->nspaces; *p_inspace; p_inspace++) {
			knspace_t *knspace = NULL;
			inspace_t *inspace = *p_inspace;
			knspace = inspace_load(inspace, error);
			if (!knspace) {
				retval = BOOL_FALSE;
				continue;
			}
			if (!kview_add_nspace(kview, knspace)) {
				faux_error_sprintf(error,
					TAG": Can't add NSPACE \"%s\"",
					knspace_view_ref(knspace));
				knspace_free(knspace);
				retval = BOOL_FALSE;
				continue;
			}
		}
	}

	// COMMAND list
	if (iview->commands) {
		icommand_t **p_icommand = NULL;
		for (p_icommand = *iview->commands; *p_icommand; p_icommand++) {
			kcommand_t *kcommand = NULL;
			icommand_t *icommand = *p_icommand;
			kcommand = icommand_load(icommand, error);
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


kview_t *iview_load(const iview_t *iview, faux_error_t *error)
{
	kview_t *kview = NULL;

	if (!iview)
		return NULL;

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

	if (!iview_parse(iview, kview, error)) {
		kview_free(kview);
		return NULL;
	}

	// Parse nested elements
	if (!iview_parse_nested(iview, kview, error)) {
		kview_free(kview);
		return NULL;
	}

	return kview;
}


char *iview_deploy(const kview_t *kview, int level)
{
	char *str = NULL;
	char *tmp = NULL;
	kview_commands_node_t *commands_iter = NULL;
	kview_nspaces_node_t *nspaces_iter = NULL;

	if (!kview)
		return NULL;

	tmp = faux_str_sprintf("%*cVIEW {\n", level, ' ');
	faux_str_cat(&str, tmp);
	faux_str_free(tmp);

	attr2ctext(&str, "name", kview_name(kview), level + 1);

	// NSPACE list
	nspaces_iter = kview_nspaces_iter(kview);
	if (nspaces_iter) {
		knspace_t *nspace = NULL;

		tmp = faux_str_sprintf("\n%*cNSPACE_LIST\n\n", level + 1, ' ');
		faux_str_cat(&str, tmp);
		faux_str_free(tmp);

		while ((nspace = kview_nspaces_each(&nspaces_iter))) {
			tmp = inspace_deploy(nspace, level + 2);
			faux_str_cat(&str, tmp);
			faux_str_free(tmp);
		}

		tmp = faux_str_sprintf("%*cEND_NSPACE_LIST,\n", level + 1, ' ');
		faux_str_cat(&str, tmp);
		faux_str_free(tmp);
	}

	// COMMAND list
	commands_iter = kview_commands_iter(kview);
	if (commands_iter) {
		kcommand_t *command = NULL;

		tmp = faux_str_sprintf("\n%*cCOMMAND_LIST\n\n", level + 1, ' ');
		faux_str_cat(&str, tmp);
		faux_str_free(tmp);

		while ((command = kview_commands_each(&commands_iter))) {
			tmp = icommand_deploy(command, level + 2);
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
