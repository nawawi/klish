#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/faux.h>
#include <faux/str.h>
#include <faux/list.h>
#include <klish/khelper.h>
#include <klish/kaction.h>
#include <klish/kentry.h>


struct kentry_s {
	char *name; // Mandatory name (identifier within entries tree)
	char *help; // Help for the entry
	kentry_t *parent; // Parent kentry_t element
	bool_t container; // Is entry container (element with hidden path)
	kentry_mode_e mode; // Mode of nested ENTRYs list
	size_t min; // Min occurs of entry
	size_t max; // Max occurs of entry
	char *ptype_str; // Text reference to PTYPE
	kentry_t *ptype; // Resolved entry's PTYPE
	char *ref_str; // Text reference to aliased ENTRY
	kentry_t *ref; // Resolved aliased ENTRY
	char *value; // Additional info
	bool_t restore; // Should entry restore its depth while execution
	faux_list_t *entrys; // Nested ENTRYs
	faux_list_t *actions; // Nested ACTIONs
};


// Simple methods

// Name
KGET_STR(entry, name);

// Help
KGET_STR(entry, help);
KSET_STR(entry, help);

// Parent
KGET(entry, kentry_t *, parent);
KSET(entry, kentry_t *, parent);

// Container
KGET_BOOL(entry, container);
KSET_BOOL(entry, container);

// Mode
KGET(entry, kentry_mode_e, mode);
KSET(entry, kentry_mode_e, mode);

// Min occurs
KGET(entry, size_t, min);
KSET(entry, size_t, min);

// Max occurs
KGET(entry, size_t, max);
KSET(entry, size_t, max);

// PTYPE string (must be resolved later)
KGET_STR(entry, ptype_str);
KSET_STR(entry, ptype_str);
// PTYPE (resolved)
KGET(entry, kentry_t *, ptype);
KSET(entry, kentry_t *, ptype);

// Ref string (must be resolved later)
KGET_STR(entry, ref_str);
KSET_STR(entry, ref_str);
// Ref (resolved)
KGET(entry, kentry_t *, ref);
KSET(entry, kentry_t *, ref);

// Value
KGET_STR(entry, value);
KSET_STR(entry, value);

// Restore
KGET_BOOL(entry, restore);
KSET_BOOL(entry, restore);

// Nested ENTRYs list
KGET(entry, faux_list_t *, entrys);
static KCMP_NESTED(entry, entry, name);
static KCMP_NESTED_BY_KEY(entry, entry, name);
KADD_NESTED(entry, entry);
KFIND_NESTED(entry, entry);
KNESTED_ITER(entry, entry);
KNESTED_EACH(entry, entry);

// ACTION list
KGET(entry, faux_list_t *, actions);
KADD_NESTED(entry, action);
KNESTED_LEN(entry, action);
KNESTED_ITER(entry, action);
KNESTED_EACH(entry, action);


kentry_t *kentry_new(const char *name)
{
	kentry_t *entry = NULL;

	if (faux_str_is_empty(name))
		return NULL;

	entry = faux_zmalloc(sizeof(*entry));
	assert(entry);
	if (!entry)
		return NULL;

	// Initialize
	entry->name = faux_str_dup(name);
	entry->help = NULL;
	entry->parent = NULL;
	entry->container = BOOL_FALSE;
	entry->mode = KENTRY_MODE_SWITCH;
	entry->min = 1;
	entry->max = 1;
	entry->ptype_str = NULL;
	entry->ptype = NULL;
	entry->ref_str = NULL;
	entry->ref = NULL;
	entry->value = NULL;
	entry->restore = BOOL_FALSE;

	// ENTRY list
	entry->entrys = faux_list_new(FAUX_LIST_UNSORTED, FAUX_LIST_UNIQUE,
		kentry_entry_compare, kentry_entry_kcompare,
		(void (*)(void *))kentry_free);
	assert(entry->entrys);

	// ACTION list
	entry->actions = faux_list_new(FAUX_LIST_UNSORTED, FAUX_LIST_NONUNIQUE,
		NULL, NULL, (void (*)(void *))kaction_free);
	assert(entry->actions);

	return entry;
}


void kentry_free(kentry_t *entry)
{
	if (!entry)
		return;

	faux_str_free(entry->name);
	faux_str_free(entry->help);
	faux_str_free(entry->ptype_str);
	faux_str_free(entry->ref_str);
	faux_str_free(entry->value);

	faux_list_free(entry->entrys);
	faux_list_free(entry->actions);

	faux_free(entry);
}
