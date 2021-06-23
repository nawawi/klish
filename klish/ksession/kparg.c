/** @file kparg.c
 * @brief Parsed ARGument
 */


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <faux/str.h>
#include <klish/khelper.h>
#include <klish/kentry.h>
#include <klish/kpargv.h> // Contains parg and pargv


struct kparg_s {
	kentry_t *entry;
	char *value;
};


// Entry
KGET(parg, kentry_t *, entry);

// Value
KSET_STR(parg, value);
KGET_STR(parg, value);


kparg_t *kparg_new(kentry_t *entry, const char *value)
{
	kparg_t *parg = NULL;

	if (!entry)
		return NULL;

	parg = faux_zmalloc(sizeof(*parg));
	assert(parg);
	if (!parg)
		return NULL;

	// Initialize
	parg->entry = entry;
	kparg_set_value(parg, value);

	return parg;
}


void kparg_free(kparg_t *parg)
{
	if (!parg)
		return;

	faux_str_free(parg->value);

	faux_free(parg);
}
