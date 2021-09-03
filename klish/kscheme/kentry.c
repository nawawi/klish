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
	kentry_purpose_e purpose; // Special purpose of ENTRY
	size_t min; // Min occurs of entry
	size_t max; // Max occurs of entry
	char *ptype_str; // Text reference to PTYPE
	kentry_t *ptype; // Resolved entry's PTYPE
	char *ref_str; // Text reference to aliased ENTRY
	char *value; // Additional info
	bool_t restore; // Should entry restore its depth while execution
	bool_t order; // Is entry ordered
	bool_t filter; // Is entry filter. Filter can't have inline actions.
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

// Purpose
KGET(entry, kentry_purpose_e, purpose);
KSET(entry, kentry_purpose_e, purpose);

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

// Value
KGET_STR(entry, value);
KSET_STR(entry, value);

// Restore
KGET_BOOL(entry, restore);
KSET_BOOL(entry, restore);

// Order
KGET_BOOL(entry, order);
KSET_BOOL(entry, order);

// Filter
KGET_BOOL(entry, filter);
KSET_BOOL(entry, filter);

// Nested ENTRYs list
KGET(entry, faux_list_t *, entrys);
static KCMP_NESTED(entry, entry, name);
static KCMP_NESTED_BY_KEY(entry, entry, name);
KADD_NESTED(entry, kentry_t *, entrys);
KFIND_NESTED(entry, entry);
KNESTED_LEN(entry, entrys);
KNESTED_IS_EMPTY(entry, entrys);
KNESTED_ITER(entry, entrys);
KNESTED_EACH(entry, kentry_t *, entrys);

// ACTION list
KGET(entry, faux_list_t *, actions);
KADD_NESTED(entry, kaction_t *, actions);
KNESTED_LEN(entry, actions);
KNESTED_ITER(entry, actions);
KNESTED_EACH(entry, kaction_t *, actions);


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
	entry->mode = KENTRY_MODE_SEQUENCE;
	entry->purpose = KENTRY_PURPOSE_COMMON;
	entry->min = 1;
	entry->max = 1;
	entry->ptype_str = NULL;
	entry->ptype = NULL;
	entry->ref_str = NULL;
	entry->value = NULL;
	entry->restore = BOOL_FALSE;
	entry->order = BOOL_FALSE;
	entry->filter = BOOL_FALSE;

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


static void kentry_free_non_link(kentry_t *entry)
{
	if (!entry)
		return;

	faux_str_free(entry->ptype_str);

	faux_list_free(entry->entrys);
	faux_list_free(entry->actions);
}


static void kentry_free_common(kentry_t *entry)
{
	if (!entry)
		return;

	faux_str_free(entry->name);
	faux_str_free(entry->value);
	faux_str_free(entry->help);
	faux_str_free(entry->ref_str);
}


void kentry_free(kentry_t *entry)
{
	if (!entry)
		return;

	// If ENTRY is not a link
	if (!kentry_ref_str(entry))
		kentry_free_non_link(entry);

	// For links and non-links
	kentry_free_common(entry);

	faux_free(entry);
}


bool_t kentry_link(kentry_t *dst, const kentry_t *src)
{
	assert(dst);
	if (!dst)
		return BOOL_FALSE;
	assert(src);
	if (!src)
		return BOOL_FALSE;

	// Free all fields that will be linker to src later
	kentry_free_non_link(dst);

	// Copy structure by hand because else some fields must be
	// returned back anyway and temp memory must be allocated. I think it
	// worse.
	// name - orig
	// help - orig
	// parent - orig
	// container - orig
	dst->mode = src->mode;
	// purpose - orig
	// min - orig
	// max - orig
	dst->ptype_str = src->ptype_str;
	dst->ptype = src->ptype;
	// ref_str - orig
	// value - orig
	// restore - orig
	// order - orig
	dst->filter = src->filter;
	dst->entrys = src->entrys;
	dst->actions = src->actions;

	return BOOL_TRUE;
}
