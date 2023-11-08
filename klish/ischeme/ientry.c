#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <faux/list.h>
#include <faux/conv.h>
#include <klish/khelper.h>
#include <klish/ientry.h>
#include <klish/kentry.h>

#define TAG "ENTRY"


bool_t ientry_parse(const ientry_t *info, kentry_t *entry, faux_error_t *error)
{
	bool_t retcode = BOOL_TRUE;

	if (!info)
		return BOOL_FALSE;
	if (!entry)
		return BOOL_FALSE;

	// Help
	if (!faux_str_is_empty(info->help)) {
		if (!kentry_set_help(entry, info->help)) {
			faux_error_add(error, TAG": Illegal 'help' attribute");
			retcode = BOOL_FALSE;
		}
	}

	// Container
	if (!faux_str_is_empty(info->container)) {
		bool_t b = BOOL_FALSE;
		if (!faux_conv_str2bool(info->container, &b) ||
			!kentry_set_container(entry, b)) {
			faux_error_add(error, TAG": Illegal 'container' attribute");
			retcode = BOOL_FALSE;
		}
	}

	// Mode
	if (!faux_str_is_empty(info->mode)) {
		kentry_mode_e mode = KENTRY_MODE_NONE;
		if (!faux_str_casecmp(info->mode, "sequence"))
			mode = KENTRY_MODE_SEQUENCE;
		else if (!faux_str_casecmp(info->mode, "switch"))
			mode = KENTRY_MODE_SWITCH;
		else if (!faux_str_casecmp(info->mode, "empty"))
			mode = KENTRY_MODE_EMPTY;
		if ((KENTRY_MODE_NONE == mode) || !kentry_set_mode(entry, mode)) {
			faux_error_add(error, TAG": Illegal 'mode' attribute");
			retcode = BOOL_FALSE;
		}
	}

	// Purpose
	if (!faux_str_is_empty(info->purpose)) {
		kentry_purpose_e purpose = KENTRY_PURPOSE_NONE;
		if (!faux_str_casecmp(info->purpose, "common"))
			purpose = KENTRY_PURPOSE_COMMON;
		else if (!faux_str_casecmp(info->purpose, "ptype"))
			purpose = KENTRY_PURPOSE_PTYPE;
		else if (!faux_str_casecmp(info->purpose, "prompt"))
			purpose = KENTRY_PURPOSE_PROMPT;
		else if (!faux_str_casecmp(info->purpose, "cond"))
			purpose = KENTRY_PURPOSE_COND;
		else if (!faux_str_casecmp(info->purpose, "completion"))
			purpose = KENTRY_PURPOSE_COMPLETION;
		else if (!faux_str_casecmp(info->purpose, "help"))
			purpose = KENTRY_PURPOSE_HELP;
		else if (!faux_str_casecmp(info->purpose, "log"))
			purpose = KENTRY_PURPOSE_LOG;
		if ((KENTRY_PURPOSE_NONE == purpose) || !kentry_set_purpose(entry, purpose)) {
			faux_error_add(error, TAG": Illegal 'purpose' attribute");
			retcode = BOOL_FALSE;
		}
	}

	// Min occurs
	if (!faux_str_is_empty(info->min)) {
		unsigned int i = 0;
		if (!faux_conv_atoui(info->min, &i, 0) ||
			!kentry_set_min(entry, (size_t)i)) {
			faux_error_add(error, TAG": Illegal 'min' attribute");
			retcode = BOOL_FALSE;
		}
	}

	// Max occurs
	if (!faux_str_is_empty(info->max)) {
		unsigned int i = 0;
		if (!faux_conv_atoui(info->max, &i, 0) ||
			!kentry_set_max(entry, (size_t)i)) {
			faux_error_add(error, TAG": Illegal 'max' attribute");
			retcode = BOOL_FALSE;
		}
	}

	// Ref string
	if (!faux_str_is_empty(info->ref)) {
		if (!kentry_set_ref_str(entry, info->ref)) {
			faux_error_add(error, TAG": Illegal 'ref' attribute");
			retcode = BOOL_FALSE;
		}
	}

	// Value
	if (!faux_str_is_empty(info->value)) {
		if (!kentry_set_value(entry, info->value)) {
			faux_error_add(error, TAG": Illegal 'value' attribute");
			retcode = BOOL_FALSE;
		}
	}

	// Restore
	if (!faux_str_is_empty(info->restore)) {
		bool_t b = BOOL_FALSE;
		if (!faux_conv_str2bool(info->restore, &b) ||
			!kentry_set_restore(entry, b)) {
			faux_error_add(error, TAG": Illegal 'restore' attribute");
			retcode = BOOL_FALSE;
		}
	}

	// Order
	if (!faux_str_is_empty(info->order)) {
		bool_t b = BOOL_FALSE;
		if (!faux_conv_str2bool(info->order, &b) ||
			!kentry_set_order(entry, b)) {
			faux_error_add(error, TAG": Illegal 'order' attribute");
			retcode = BOOL_FALSE;
		}
	}

	// Filter
	if (!faux_str_is_empty(info->filter)) {
		kentry_filter_e filter = KENTRY_FILTER_NONE;
		if (!faux_str_casecmp(info->filter, "false"))
			filter = KENTRY_FILTER_FALSE;
		else if (!faux_str_casecmp(info->filter, "true"))
			filter = KENTRY_FILTER_TRUE;
		else if (!faux_str_casecmp(info->filter, "dual"))
			filter = KENTRY_FILTER_DUAL;
		if ((KENTRY_FILTER_NONE == filter) || !kentry_set_filter(entry, filter)) {
			faux_error_add(error, TAG": Illegal 'filter' attribute");
			retcode = BOOL_FALSE;
		}
	}

	return retcode;
}


bool_t ientry_parse_nested(const ientry_t *ientry, kentry_t *kentry,
	faux_error_t *error)
{
	bool_t retval = BOOL_TRUE;

	if (!kentry || !ientry) {
		faux_error_add(error, TAG": Internal error");
		return BOOL_FALSE;
	}

	// ENTRY list
	// ENTRYs can be duplicate. Duplicated ENTRY will add nested
	// elements to existent ENTRY. Also it can overwrite ENTRY attributes.
	// So there is no special rule which attribute value will be "on top".
	// It's a random. Technically later ENTRYs will rewrite previous
	// values.
	if (ientry->entrys) {
		ientry_t **p_ientry = NULL;
		for (p_ientry = *ientry->entrys; *p_ientry; p_ientry++) {
			kentry_t *nkentry = NULL;
			ientry_t *nientry = *p_ientry;
			const char *entry_name = nientry->name;

			if (entry_name)
				nkentry = kentry_find_entry(kentry, entry_name);

			// ENTRY already exists
			if (nkentry) {
				if (!ientry_parse(nientry, nkentry, error)) {
					retval = BOOL_FALSE;
					continue;
				}
				if (!ientry_parse_nested(nientry, nkentry,
					error)) {
					retval = BOOL_FALSE;
					continue;
				}
				continue;
			}

			// New ENTRY
			nkentry = ientry_load(nientry, error);
			if (!nkentry) {
				retval = BOOL_FALSE;
				continue;
			}
			kentry_set_parent(nkentry, kentry); // Set parent entry
			if (!kentry_add_entrys(kentry, nkentry)) {
				faux_error_sprintf(error,
					TAG": Can't add ENTRY \"%s\"",
					kentry_name(nkentry));
				kentry_free(nkentry);
				retval = BOOL_FALSE;
				continue;
			}
		}
	}

	// ACTION list
	if (ientry->actions) {
		iaction_t **p_iaction = NULL;
		for (p_iaction = *ientry->actions; *p_iaction; p_iaction++) {
			kaction_t *kaction = NULL;
			iaction_t *iaction = *p_iaction;

			kaction = iaction_load(iaction, error);
			if (!kaction) {
				retval = BOOL_FALSE;
				continue;
			}
			if (!kentry_add_actions(kentry, kaction)) {
				faux_error_sprintf(error,
					TAG": Can't add ACTION #%d",
					kentry_actions_len(kentry) + 1);
				kaction_free(kaction);
				retval = BOOL_FALSE;
				continue;
			}
		}
	}

	// HOTKEY list
	if (ientry->hotkeys) {
		ihotkey_t **p_ihotkey = NULL;
		for (p_ihotkey = *ientry->hotkeys; *p_ihotkey; p_ihotkey++) {
			khotkey_t *khotkey = NULL;
			ihotkey_t *ihotkey = *p_ihotkey;

			khotkey = ihotkey_load(ihotkey, error);
			if (!khotkey) {
				retval = BOOL_FALSE;
				continue;
			}
			if (!kentry_add_hotkeys(kentry, khotkey)) {
				faux_error_sprintf(error,
					TAG": Can't add HOTKEY \"%s\"",
					khotkey_key(khotkey));
				khotkey_free(khotkey);
				retval = BOOL_FALSE;
				continue;
			}
		}
	}

	if (!retval)
		faux_error_sprintf(error, TAG" \"%s\": Illegal nested elements",
			kentry_name(kentry));

	return retval;
}


kentry_t *ientry_load(const ientry_t *ientry, faux_error_t *error)
{
	kentry_t *kentry = NULL;

	if (!ientry)
		return NULL;

	// Name [mandatory]
	if (faux_str_is_empty(ientry->name)) {
		faux_error_add(error, TAG": Empty 'name' attribute");
		return NULL;
	}

	kentry = kentry_new(ientry->name);
	if (!kentry) {
		faux_error_sprintf(error, TAG" \"%s\": Can't create object",
			ientry->name);
		return NULL;
	}

	if (!ientry_parse(ientry, kentry, error)) {
		kentry_free(kentry);
		return NULL;
	}

	// Parse nested elements
	if (!ientry_parse_nested(ientry, kentry, error)) {
		kentry_free(kentry);
		return NULL;
	}

	return kentry;
}


char *ientry_deploy(const kentry_t *kentry, int level)
{
	char *str = NULL;
	char *tmp = NULL;
	char *mode = NULL;
	char *purpose = NULL;
	char *filter = NULL;
	kentry_entrys_node_t *entrys_iter = NULL;
	kentry_actions_node_t *actions_iter = NULL;
	kentry_hotkeys_node_t *hotkeys_iter = NULL;
	char *num = NULL;

	tmp = faux_str_sprintf("%*cENTRY {\n", level, ' ');
	faux_str_cat(&str, tmp);
	faux_str_free(tmp);

	attr2ctext(&str, "name", kentry_name(kentry), level + 1);
	attr2ctext(&str, "help", kentry_help(kentry), level + 1);
	attr2ctext(&str, "ref", kentry_ref_str(kentry), level + 1);

	// Links (ENTRY with 'ref' attribute) doesn't need the following filds
	// that will be replaced by content of referenced ENTRY
	if (faux_str_is_empty(kentry_ref_str(kentry))) {

		attr2ctext(&str, "container", faux_conv_bool2str(kentry_container(kentry)), level + 1);

		// Mode
		switch (kentry_mode(kentry)) {
		case KENTRY_MODE_SEQUENCE:
			mode = "sequence";
			break;
		case KENTRY_MODE_SWITCH:
			mode = "switch";
			break;
		case KENTRY_MODE_EMPTY:
			mode = "empty";
			break;
		default:
			mode = NULL;
		}
		attr2ctext(&str, "mode", mode, level + 1);

		// Purpose
		switch (kentry_purpose(kentry)) {
		case KENTRY_PURPOSE_COMMON:
			purpose = "common";
			break;
		case KENTRY_PURPOSE_PTYPE:
			purpose = "ptype";
			break;
		case KENTRY_PURPOSE_PROMPT:
			purpose = "prompt";
			break;
		case KENTRY_PURPOSE_COND:
			purpose = "cond";
			break;
		case KENTRY_PURPOSE_COMPLETION:
			purpose = "completion";
			break;
		case KENTRY_PURPOSE_HELP:
			purpose = "help";
			break;
		case KENTRY_PURPOSE_LOG:
			purpose = "log";
			break;
		default:
			purpose = NULL;
		}
		attr2ctext(&str, "purpose", purpose, level + 1);

		// Min occurs
		num = faux_str_sprintf("%u", kentry_min(kentry));
		attr2ctext(&str, "min", num, level + 1);
		faux_str_free(num);
		num = NULL;

		// Max occurs
		num = faux_str_sprintf("%u", kentry_max(kentry));
		attr2ctext(&str, "max", num, level + 1);
		faux_str_free(num);
		num = NULL;

		attr2ctext(&str, "value", kentry_value(kentry), level + 1);
		attr2ctext(&str, "restore", faux_conv_bool2str(kentry_restore(kentry)), level + 1);
		attr2ctext(&str, "order", faux_conv_bool2str(kentry_order(kentry)), level + 1);

		// Filter
		switch (kentry_filter(kentry)) {
		case KENTRY_FILTER_FALSE:
			filter = "false";
			break;
		case KENTRY_FILTER_TRUE:
			filter = "true";
			break;
		case KENTRY_FILTER_DUAL:
			filter = "dual";
			break;
		default:
			filter = NULL;
		}
		attr2ctext(&str, "filter", filter, level + 1);

		// ENTRY list
		entrys_iter = kentry_entrys_iter(kentry);
		if (entrys_iter) {
			kentry_t *nentry = NULL;

			tmp = faux_str_sprintf("\n%*cENTRY_LIST\n\n", level + 1, ' ');
			faux_str_cat(&str, tmp);
			faux_str_free(tmp);

			while ((nentry = kentry_entrys_each(&entrys_iter))) {
				tmp = ientry_deploy(nentry, level + 2);
				faux_str_cat(&str, tmp);
				faux_str_free(tmp);
			}

			tmp = faux_str_sprintf("%*cEND_ENTRY_LIST,\n", level + 1, ' ');
			faux_str_cat(&str, tmp);
			faux_str_free(tmp);
		}

		// ACTION list
		actions_iter = kentry_actions_iter(kentry);
		if (actions_iter) {
			kaction_t *action = NULL;

			tmp = faux_str_sprintf("\n%*cACTION_LIST\n\n", level + 1, ' ');
			faux_str_cat(&str, tmp);
			faux_str_free(tmp);

			while ((action = kentry_actions_each(&actions_iter))) {
				tmp = iaction_deploy(action, level + 2);
				faux_str_cat(&str, tmp);
				faux_str_free(tmp);
			}

			tmp = faux_str_sprintf("%*cEND_ACTION_LIST,\n", level + 1, ' ');
			faux_str_cat(&str, tmp);
			faux_str_free(tmp);
		}

		// HOTKEY list
		hotkeys_iter = kentry_hotkeys_iter(kentry);
		if (hotkeys_iter) {
			khotkey_t *hotkey = NULL;

			tmp = faux_str_sprintf("\n%*cHOTKEY_LIST\n\n", level + 1, ' ');
			faux_str_cat(&str, tmp);
			faux_str_free(tmp);

			while ((hotkey = kentry_hotkeys_each(&hotkeys_iter))) {
				tmp = ihotkey_deploy(hotkey, level + 2);
				faux_str_cat(&str, tmp);
				faux_str_free(tmp);
			}

			tmp = faux_str_sprintf("%*cEND_HOTKEY_LIST,\n", level + 1, ' ');
			faux_str_cat(&str, tmp);
			faux_str_free(tmp);
		}

	} // ref_str

	tmp = faux_str_sprintf("%*c},\n\n", level, ' ');
	faux_str_cat(&str, tmp);
	faux_str_free(tmp);

	return str;
}
