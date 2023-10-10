#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <syslog.h>

#include <faux/faux.h>
#include <faux/str.h>
#include <faux/list.h>
#include <klish/khelper.h>
#include <klish/kaction.h>
#include <klish/kentry.h>
#include <klish/khotkey.h>


// WARNING: Changing this structure don't forget to update kentry_link()
struct kentry_s {
	char *name; // Mandatory name (identifier within entries tree)
	char *help; // Help for the entry
	kentry_t *parent; // Parent kentry_t element
	bool_t container; // Is entry container (element with hidden path)
	kentry_mode_e mode; // Mode of nested ENTRYs list
	kentry_purpose_e purpose; // Special purpose of ENTRY
	size_t min; // Min occurs of entry
	size_t max; // Max occurs of entry
	char *ref_str; // Text reference to aliased ENTRY
	char *value; // Additional info
	bool_t restore; // Should entry restore its depth while execution
	bool_t order; // Is entry ordered
	kentry_filter_e filter; // Is entry filter. Filter can't have inline actions.
	faux_list_t *entrys; // Nested ENTRYs
	faux_list_t *actions; // Nested ACTIONs
	faux_list_t *hotkeys; // Hotkeys
	// Fast links to nested entries with special purposes.
	kentry_t** nested_by_purpose;
	void *udata;
	kentry_udata_free_fn udata_free_fn;
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
KGET(entry, kentry_filter_e, filter);
KSET(entry, kentry_filter_e, filter);

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

// HOTKEY list
KGET(entry, faux_list_t *, hotkeys);
KCMP_NESTED(entry, hotkey, key);
KADD_NESTED(entry, khotkey_t *, hotkeys);
KNESTED_LEN(entry, hotkeys);
KNESTED_ITER(entry, hotkeys);
KNESTED_EACH(entry, khotkey_t *, hotkeys);


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
	entry->ref_str = NULL;
	entry->value = NULL;
	entry->restore = BOOL_FALSE;
	entry->order = BOOL_FALSE;
	entry->filter = KENTRY_FILTER_FALSE;
	entry->udata = NULL;
	entry->udata_free_fn = NULL;

	// ENTRY list
	entry->entrys = faux_list_new(FAUX_LIST_UNSORTED, FAUX_LIST_UNIQUE,
		kentry_entry_compare, kentry_entry_kcompare,
		(void (*)(void *))kentry_free);
	assert(entry->entrys);

	// ACTION list
	entry->actions = faux_list_new(FAUX_LIST_UNSORTED, FAUX_LIST_NONUNIQUE,
		NULL, NULL, (void (*)(void *))kaction_free);
	assert(entry->actions);

	// HOTKEY list
	entry->hotkeys = faux_list_new(FAUX_LIST_UNSORTED, FAUX_LIST_UNIQUE,
		kentry_hotkey_compare, NULL, (void (*)(void *))khotkey_free);
	assert(entry->hotkeys);

	entry->nested_by_purpose = faux_zmalloc(
		KENTRY_PURPOSE_MAX * sizeof(*(entry->nested_by_purpose)));

	return entry;
}


static void kentry_free_non_link(kentry_t *entry)
{
	if (!entry)
		return;

	faux_list_free(entry->entrys);
	faux_list_free(entry->actions);
	faux_list_free(entry->hotkeys);
	faux_free(entry->nested_by_purpose);
}


static void kentry_free_common(kentry_t *entry)
{
	if (!entry)
		return;

	faux_str_free(entry->name);
	faux_str_free(entry->value);
	faux_str_free(entry->help);
	faux_str_free(entry->ref_str);
	if (entry->udata && entry->udata_free_fn)
		entry->udata_free_fn(entry->udata);
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
	// TODO: Some fields must be copied from src if they are not defined in
	// dst explicitly. But now I don't know how to do so. These fields are:
	// container, purpose, min, max, restore, order.

	// name - orig
	// help - orig
	if (!dst->help)
		dst->help = faux_str_dup(src->help);
	// parent - orig
	// container - orig
	// mode - ref
	dst->mode = src->mode;
	// purpose - orig
	// min - orig
	// max - orig
	// ref_str - orig
	// value - orig
	if (!dst->value)
		dst->value = faux_str_dup(src->value);
	// restore - orig
	// order - orig
	// filter - ref
	dst->filter = src->filter;
	// entrys - ref
	dst->entrys = src->entrys;
	// actions - ref
	dst->actions = src->actions;
	// hotkeys - ref
	dst->hotkeys = src->hotkeys;
	// nested_by_purpose - ref
	dst->nested_by_purpose = src->nested_by_purpose;
	// udata - orig
	// udata_free_fn - orig

	return BOOL_TRUE;
}


kentry_t *kentry_nested_by_purpose(const kentry_t *entry, kentry_purpose_e purpose)
{
	assert(entry);
	if (!entry)
		return NULL;

	return entry->nested_by_purpose[purpose];
}


bool_t kentry_set_nested_by_purpose(kentry_t *entry, kentry_purpose_e purpose,
	kentry_t *nested)
{
	assert(entry);
	if (!entry)
		return BOOL_FALSE;

	entry->nested_by_purpose[purpose] = nested;

	return BOOL_TRUE;
}


void *kentry_udata(const kentry_t *entry)
{
	assert(entry);
	if (!entry)
		return NULL;

	return entry->udata;
}


bool_t kentry_set_udata(kentry_t *entry, void *data, kentry_udata_free_fn free_fn)
{
	assert(entry);
	if (!entry)
		return BOOL_FALSE;

	// Free old udata value
	if (entry->udata) {
		if (entry->udata_free_fn)
			entry->udata_free_fn(entry->udata);
		else if (free_fn)
			free_fn(entry->udata);
	}

	entry->udata = data;
	entry->udata_free_fn = free_fn;

	return BOOL_TRUE;
}


// Get integral value of "in" field of all ENTRY's actions
// false < true < tty
kaction_io_e kentry_in(const kentry_t *entry)
{
	kentry_actions_node_t *iter = NULL;
	kaction_t *action = NULL;
	kaction_io_e io = KACTION_IO_FALSE;

	if (!entry)
		return io;
	iter = kentry_actions_iter(entry);
	while ((action = kentry_actions_each(&iter))) {
		kaction_io_e cur_io = kaction_in(action);
		if (cur_io > io)
			io = cur_io;
	}

	return io;
}


// Get integral value of "out" field of all ENTRY's actions
// false < true < tty
kaction_io_e kentry_out(const kentry_t *entry)
{
	kentry_actions_node_t *iter = NULL;
	kaction_t *action = NULL;
	kaction_io_e io = KACTION_IO_FALSE;

	if (!entry)
		return io;
	iter = kentry_actions_iter(entry);
	while ((action = kentry_actions_each(&iter))) {
		kaction_io_e cur_io = kaction_out(action);
		if (cur_io > io)
			io = cur_io;
	}

	return io;
}
