#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <faux/conv.h>
#include <faux/error.h>
#include <klish/khelper.h>
#include <klish/khotkey.h>
#include <klish/ihotkey.h>

#define TAG "HOTKEY"

/*
bool_t ihotkey_parse(const ihotkey_t *info, khotkey_t *hotkey, faux_error_t *error)
{
	bool_t retcode = BOOL_TRUE;

	if (!info)
		return BOOL_FALSE;
	if (!hotkey)
		return BOOL_FALSE;

	return retcode;
}
*/

khotkey_t *ihotkey_load(const ihotkey_t *ihotkey, faux_error_t *error)
{
	khotkey_t *khotkey = NULL;

	// Key [mandatory]
	if (faux_str_is_empty(ihotkey->key)) {
		faux_error_add(error, TAG": Empty 'key' attribute");
		return NULL;
	}

	// Command [mandatory]
	if (faux_str_is_empty(ihotkey->cmd)) {
		faux_error_add(error, TAG": Empty 'cmd' attribute");
		return NULL;
	}

	khotkey = khotkey_new(ihotkey->key, ihotkey->cmd);
	if (!khotkey) {
		faux_error_add(error, TAG": Can't create object");
		return NULL;
	}
//	if (!ihotkey_parse(ihotkey, khotkey, error)) {
//		khotkey_free(khotkey);
//		return NULL;
//	}

	return khotkey;
}


char *ihotkey_deploy(const khotkey_t *khotkey, int level)
{
	char *str = NULL;
	char *tmp = NULL;

	if (!khotkey)
		return NULL;

	tmp = faux_str_sprintf("%*chotkey {\n", level, ' ');
	faux_str_cat(&str, tmp);
	faux_str_free(tmp);

	attr2ctext(&str, "key", khotkey_key(khotkey), level + 1);
	attr2ctext(&str, "cmd", khotkey_cmd(khotkey), level + 1);

	tmp = faux_str_sprintf("%*c},\n\n", level, ' ');
	faux_str_cat(&str, tmp);
	faux_str_free(tmp);

	return str;
}
