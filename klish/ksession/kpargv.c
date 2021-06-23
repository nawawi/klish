/** @file kpargv.c
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <faux/list.h>
#include <klish/khelper.h>
#include <klish/kpargv.h>


struct kpargv_s {
	faux_list_t *pargs;
};


// Pargs
KGET(pargv, faux_list_t *, pargs);
KADD_NESTED(pargv, parg);
KNESTED_LEN(pargv, parg);
KNESTED_IS_EMPTY(pargv, parg);
KNESTED_ITER(pargv, parg);
KNESTED_EACH(pargv, parg);


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


kparg_t *kpargv_pargs_last(const kpargv_t *pargv)
{
	assert(pargv);
	if (!pargv)
		return NULL;
	if (kpargv_pargs_is_empty(pargv))
		return NULL;

	return (kparg_t *)faux_list_data(faux_list_tail(pargv->pargs));
}
