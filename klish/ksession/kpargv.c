/** @file kpargv.c
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <faux/list.h>
#include <klish/khelper.h>
#include <klish/kcommand.h>
#include <klish/kparam.h>
#include <klish/kpargv.h>


struct kpargv_s {
	kcommand_t *command;
	faux_list_t *pargs;
};


// Command
KGET(pargv, kcommand_t *, command);
KSET(pargv, kcommand_t *, command);

// Pargs
KGET(pargv, faux_list_t *, pargs);


kpargv_t *kpargv_new()
{
	kpargv_t *pargv = NULL;

	pargv = faux_zmalloc(sizeof(*pargv));
	assert(pargv);
	if (!pargv)
		return NULL;

	// Parsed arguments list
	pargv->pargs = faux_list_new(FAUX_LIST_UNSORTED, FAUX_LIST_NONUNIQUE,
		NULL, NULL, (void (*)(void *))kparg_free);
	assert(pargv->pargs);

	return pargv;
}


void kpargv_free(kpargv_t *pargv)
{
	if (!pargv)
		return;

	faux_list_free(pargv->pargs);

	free(pargv);
}


size_t kpargv_len(const kpargv_t *pargv)
{
	assert(pargv);
	if (!pargv)
		return 0;

	return faux_list_len(pargv->pargs);
}


size_t kpargv_is_empty(const kpargv_t *pargv)
{
	assert(pargv);
	if (!pargv)
		return 0;

	return faux_list_is_empty(pargv->pargs);
}


bool_t kpargv_add(kpargv_t *pargv, kparg_t *parg)
{
	assert(pargv);
	assert(parg);
	if (!pargv)
		return BOOL_FALSE;
	if (!parg)
		return BOOL_FALSE;

	if (!faux_list_add(pargv->pargs, parg))
		return BOOL_FALSE;

	return BOOL_TRUE;
}


kparg_t *kpargv_last(const kpargv_t *pargv)
{
	assert(pargv);
	if (!pargv)
		return NULL;
	if (kpargv_is_empty(pargv))
		return NULL;

	return (kparg_t *)faux_list_data(faux_list_tail(pargv->pargs));
}
