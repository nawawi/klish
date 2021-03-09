#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <faux/list.h>
#include <faux/error.h>
#include <klish/khelper.h>
#include <klish/kptype.h>
#include <klish/kaction.h>
#include <klish/iptype.h>

#define TAG "PTYPE"


bool_t iptype_parse(const iptype_t *info, kptype_t *ptype, faux_error_t *error)
{
	bool_t retcode = BOOL_TRUE;

	if (!info)
		return BOOL_FALSE;
	if (!ptype)
		return BOOL_FALSE;

	// Help
	if (!faux_str_is_empty(info->name)) {
		if (!kptype_set_help(ptype, info->help)) {
			faux_error_add(error, TAG": Illegal 'help' attribute");
			retcode = BOOL_FALSE;
		}
	}

	return retcode;
}


bool_t iptype_parse_nested(const iptype_t *iptype, kptype_t *kptype,
	faux_error_t *error)
{
	bool_t retval = BOOL_TRUE;

	if (!kptype || !iptype) {
		faux_error_add(error, TAG": Internal error");
		return BOOL_FALSE;
	}

	// ACTION list
	if (iptype->actions) {
		iaction_t **p_iaction = NULL;
		for (p_iaction = *iptype->actions; *p_iaction; p_iaction++) {
			kaction_t *kaction = NULL;
			iaction_t *iaction = *p_iaction;

			kaction = iaction_load(iaction, error);
			if (!kaction) {
				retval = BOOL_FALSE;
				continue;
			}
			if (!kptype_add_action(kptype, kaction)) {
				faux_error_sprintf(error,
					TAG": Can't add ACTION #%d",
					kptype_actions_len(kptype) + 1);
				kaction_free(kaction);
				retval = BOOL_FALSE;
				continue;
			}
		}
	}

	if (!retval)
		faux_error_sprintf(error,
			"PTYPE \"%s\": Illegal nested elements",
			kptype_name(kptype));

	return retval;
}


kptype_t *iptype_load(const iptype_t *iptype, faux_error_t *error)
{
	kptype_t *kptype = NULL;

	if (!iptype)
		return NULL;

	// Name [mandatory]
	if (faux_str_is_empty(iptype->name)) {
		faux_error_add(error, TAG": Empty 'name' attribute");
		return NULL;
	}

	kptype = kptype_new(iptype->name);
	if (!kptype) {
		faux_error_sprintf(error, TAG" \"%s\": Can't create object",
			iptype->name);
		return NULL;
	}

	if (!iptype_parse(iptype, kptype, error)) {
		kptype_free(kptype);
		return NULL;
	}

	// Parse nested elements
	if (!iptype_parse_nested(iptype, kptype, error)) {
		kptype_free(kptype);
		return NULL;
	}

	return kptype;
}


char *iptype_deploy(const kptype_t *kptype, int level)
{
	char *str = NULL;
	char *tmp = NULL;
	kptype_actions_node_t *actions_iter = NULL;

	tmp = faux_str_sprintf("%*cPTYPE {\n", level, ' ');
	faux_str_cat(&str, tmp);
	faux_str_free(tmp);

	attr2ctext(&str, "name", kptype_name(kptype), level + 1);
	attr2ctext(&str, "help", kptype_help(kptype), level + 1);

	// ACTION list
	actions_iter = kptype_actions_iter(kptype);
	if (actions_iter) {
		kaction_t *action = NULL;

		tmp = faux_str_sprintf("\n%*cACTION_LIST\n\n", level + 1, ' ');
		faux_str_cat(&str, tmp);
		faux_str_free(tmp);

		while ((action = kptype_actions_each(&actions_iter))) {
			tmp = iaction_deploy(action, level + 2);
			faux_str_cat(&str, tmp);
			faux_str_free(tmp);
		}

		tmp = faux_str_sprintf("%*cEND_ACTION_LIST,\n", level + 1, ' ');
		faux_str_cat(&str, tmp);
		faux_str_free(tmp);
	}

	tmp = faux_str_sprintf("%*c},\n\n", level, ' ');
	faux_str_cat(&str, tmp);
	faux_str_free(tmp);

	return str;
}
