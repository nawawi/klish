#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <klish/khelper.h>
#include <klish/khotkey.h>


struct khotkey_s {
	char *key;
	char *cmd;
};


// Simple methods

// Key
KGET_STR(hotkey, key);

// Command
KGET_STR(hotkey, cmd);


khotkey_t *khotkey_new(const char *key, const char *cmd)
{
	khotkey_t *hotkey = NULL;

	// Mandatory key
	if (!key)
		return NULL;
	// Mandatory cmd
	if (!cmd)
		return NULL;

	hotkey = faux_zmalloc(sizeof(*hotkey));
	assert(hotkey);
	if (!hotkey)
		return NULL;

	// Initialize
	hotkey->key = faux_str_dup(key);
	hotkey->cmd = faux_str_dup(cmd);

	return hotkey;
}


void khotkey_free(khotkey_t *hotkey)
{
	if (!hotkey)
		return;

	faux_str_free(hotkey->key);
	faux_str_free(hotkey->cmd);

	faux_free(hotkey);
}
