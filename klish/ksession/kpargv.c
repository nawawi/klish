/** @file kpargv.c
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <faux/list.h>
#include <klish/khelper.h>
#include <klish/kentry.h>
#include <klish/kpargv.h>


struct kpargv_s {
	faux_list_t *pargs;
	faux_list_t *completions;
	kpargv_status_e status; // Parse status
	size_t level; // Number of path's level where command was found
	const kentry_t *command; // ENTRY that consider as command (has ACTIONs)
	bool_t continuable; // Last argument can be expanded
};

// Level
KGET(pargv, kpargv_status_e, status);
KSET(pargv, kpargv_status_e, status);

// Level
KGET(pargv, size_t, level);
KSET(pargv, size_t, level);

// Command
KGET(pargv, const kentry_t *, command);
KSET(pargv, const kentry_t *, command);

// Continuable
KGET_BOOL(pargv, continuable);
KSET_BOOL(pargv, continuable);

// Pargs
KGET(pargv, faux_list_t *, pargs);
KADD_NESTED(pargv, kparg_t *, pargs);
KNESTED_LEN(pargv, pargs);
KNESTED_IS_EMPTY(pargv, pargs);
KNESTED_ITER(pargv, pargs);
KNESTED_EACH(pargv, kparg_t *, pargs);

// Completions
KGET(pargv, faux_list_t *, completions);
KADD_NESTED(pargv, kentry_t *, completions);
KNESTED_LEN(pargv, completions);
KNESTED_IS_EMPTY(pargv, completions);
KNESTED_ITER(pargv, completions);
KNESTED_EACH(pargv, kentry_t *, completions);


int kpargv_pargs_kcompare(const void *key, const void *list_item)
{
	const kentry_t *f = (const kentry_t *)key;
	const kparg_t *s = (const kparg_t *)list_item;
	if (f == kparg_entry(s))
		return 0;
	return 1;
}


kpargv_t *kpargv_new()
{
	kpargv_t *pargv = NULL;

	pargv = faux_zmalloc(sizeof(*pargv));
	assert(pargv);
	if (!pargv)
		return NULL;

	// Initialization
	pargv->status = KPARSE_NONE;
	pargv->level = 0;
	pargv->command = NULL;

	// Parsed arguments list
	pargv->pargs = faux_list_new(FAUX_LIST_UNSORTED, FAUX_LIST_NONUNIQUE,
		NULL, kpargv_pargs_kcompare, (void (*)(void *))kparg_free);
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


kparg_t *kpargv_entry_exists(const kpargv_t *pargv, const void *entry)
{
	assert(pargv);
	if (!pargv)
		return NULL;
	assert(entry);
	if (!entry)
		return NULL;

	return (kparg_t *)faux_list_kfind(pargv->pargs, entry);
}


const char *kpargv_status_str(const kpargv_t *pargv)
{
	const char *s = "Unknown";

	assert(pargv);
	if (!pargv)
		return NULL;

	switch (kpargv_status(pargv)) {
	case KPARSE_NONE:
		s = "None";
		break;
	case KPARSE_OK:
		s = "Ok";
		break;
	case KPARSE_INPROGRESS:
		s = "In progress";
		break;
	case KPARSE_NOTFOUND:
		s = "Not found";
		break;
	case KPARSE_INCOMPLETED:
		s = "Incompleted";
		break;
	case KPARSE_ILLEGAL:
		s = "Illegal";
		break;
	case KPARSE_ERROR:
		s = "Error";
		break;
	}

	return s;
}
