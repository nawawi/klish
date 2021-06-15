/** @file kparg.c
 */


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <faux/str.h>
#include <klish/khelper.h>
#include <klish/kparam.h>
#include <klish/kpargv.h> // Contains parg and pargv


struct kparg_s {
	kparam_t *param;
	char *value;
};


// Param
KGET(parg, kparam_t *, param);

// Value
KSET_STR(parg, value);
KGET_STR(parg, value);


kparg_t *kparg_new(kparam_t *param, const char *value)
{
	kparg_t *parg = NULL;

	if (!param)
		return NULL;

	parg = faux_zmalloc(sizeof(*parg));
	assert(parg);
	if (!parg)
		return NULL;

	// Initialize
	parg->param = param;
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
