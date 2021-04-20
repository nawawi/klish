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


struct kptype_s {
	char *name;
	char *help;
	faux_list_t *actions;
};


// Simple methods

// Name
KGET_STR(ptype, name);

// Help
KGET_STR(ptype, help);
KSET_STR(ptype, help);

// ACTION list
KGET(ptype, faux_list_t *, actions);
KADD_NESTED(ptype, action);
KNESTED_LEN(ptype, action);
KNESTED_ITER(ptype, action);
KNESTED_EACH(ptype, action);


kptype_t *kptype_new(const char *name)
{
	kptype_t *ptype = NULL;

	if (faux_str_is_empty(name))
		return NULL;

	ptype = faux_zmalloc(sizeof(*ptype));
	assert(ptype);
	if (!ptype)
		return NULL;

	// Initialize
	ptype->name = faux_str_dup(name);
	ptype->help = NULL;

	// ACTION list
	ptype->actions = faux_list_new(FAUX_LIST_UNSORTED, FAUX_LIST_NONUNIQUE,
		NULL, NULL, (void (*)(void *))kaction_free);
	assert(ptype->actions);

	return ptype;
}


void kptype_free(kptype_t *ptype)
{
	if (!ptype)
		return;

	faux_str_free(ptype->name);
	faux_str_free(ptype->help);
	faux_list_free(ptype->actions);

	faux_free(ptype);
}
