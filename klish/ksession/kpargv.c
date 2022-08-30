/** @file kpargv.c
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <faux/list.h>
#include <faux/error.h>
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
	kpargv_purpose_e purpose; // Exec/Completion/Help
	char *last_arg;
	kparg_t *candidate_parg; // Don't free
};

// Status
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

// Purpose
KGET(pargv, kpargv_purpose_e, purpose);
KSET(pargv, kpargv_purpose_e, purpose);

// Last argument
KSET_STR(pargv, last_arg);
KGET_STR(pargv, last_arg);

// Level
KGET(pargv, kparg_t *, candidate_parg);
KSET(pargv, kparg_t *, candidate_parg);

// Pargs
KGET(pargv, faux_list_t *, pargs);
KADD_NESTED(pargv, kparg_t *, pargs);
KNESTED_LEN(pargv, pargs);
KNESTED_IS_EMPTY(pargv, pargs);
KNESTED_ITER(pargv, pargs);
KNESTED_ITERR(pargv, pargs);
KNESTED_EACH(pargv, kparg_t *, pargs);
KNESTED_EACHR(pargv, kparg_t *, pargs);

// Completions
KGET(pargv, faux_list_t *, completions);
KADD_NESTED(pargv, const kentry_t *, completions);
KNESTED_LEN(pargv, completions);
KNESTED_IS_EMPTY(pargv, completions);
KNESTED_ITER(pargv, completions);
KNESTED_EACH(pargv, const kentry_t *, completions);


static int kpargv_completions_compare(const void *first, const void *second)
{
	const kentry_t *f = (const kentry_t *)first;
	const kentry_t *s = (const kentry_t *)second;
	return strcmp(kentry_name(f), kentry_name(s));
}


static int kpargv_pargs_kcompare(const void *key, const void *list_item)
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
	pargv->continuable = BOOL_FALSE;
	pargv->purpose = KPURPOSE_EXEC;
	pargv->last_arg = NULL;
	pargv->candidate_parg = NULL;

	// Parsed arguments list
	pargv->pargs = faux_list_new(FAUX_LIST_UNSORTED, FAUX_LIST_NONUNIQUE,
		NULL, kpargv_pargs_kcompare, (void (*)(void *))kparg_free);
	assert(pargv->pargs);

	// Completions
	pargv->completions = faux_list_new(FAUX_LIST_UNSORTED, FAUX_LIST_UNIQUE,
		kpargv_completions_compare, NULL, NULL);
	assert(pargv->completions);

	return pargv;
}


void kpargv_free(kpargv_t *pargv)
{
	if (!pargv)
		return;

	faux_str_free(pargv->last_arg);

	faux_list_free(pargv->pargs);
	faux_list_free(pargv->completions);

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


const char *kpargv_status_decode(kpargv_status_e status)
{
	const char *s = "Unknown";

	switch (status) {
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
	case KPARSE_NOACTION:
		s = "Has no action";
		break;
	default: // ERROR/MAX/NONE
		s = "Error";
		break;
	}

	return s;
}


const char *kpargv_status_str(const kpargv_t *pargv)
{
	assert(pargv);
	if (!pargv)
		return NULL;

	return kpargv_status_decode(kpargv_status(pargv));
}


bool_t kpargv_accept_candidate_parg(kpargv_t *pargv)
{
	kparg_t *candidate = NULL;

	assert(pargv);
	if (!pargv)
		return BOOL_FALSE;
	if (!(candidate = pargv->candidate_parg))
		return BOOL_FALSE;
	pargv->candidate_parg = NULL;

	return kpargv_add_pargs(pargv, candidate);
}


bool_t kpargv_decline_candidate_parg(kpargv_t *pargv)
{
	assert(pargv);
	if (!pargv)
		return BOOL_FALSE;
	pargv->candidate_parg = NULL;

	return BOOL_TRUE;
}


bool_t kpargv_debug(const kpargv_t *pargv)
{
#ifdef PARGV_DEBUG
	kpargv_pargs_node_t *p_iter = NULL;

	assert(pargv);
	if (!pargv)
		return BOOL_FALSE;

	printf("Level: %lu, Command: %s, Status: %s\n",
		kpargv_level(pargv),
		kpargv_command(pargv) ? kentry_name(kpargv_command(pargv)) : "<none>",
		kpargv_status_str(pargv));

	// Parsed parameters
	p_iter = kpargv_pargs_iter(pargv);
	if (kpargv_pargs_len(pargv) > 0) {
		kparg_t *parg = NULL;
		while ((parg = kpargv_pargs_each(&p_iter))) {
			printf("%s(%s) ", kparg_value(parg), kentry_name(kparg_entry(parg)));
		}
		printf("\n");
	}

	// Completions
	if (!kpargv_completions_is_empty(pargv)) {
		const kentry_t *completion = NULL;
		kpargv_completions_node_t *citer = kpargv_completions_iter(pargv);
		printf("Completions (%s):\n", kpargv_last_arg(pargv));
		while ((completion = kpargv_completions_each(&citer)))
			printf("* %s\n", kentry_name(completion));
	}

	return BOOL_TRUE;
#else
	pargv = pargv; // Happy compiler

	return BOOL_TRUE;
#endif
}
