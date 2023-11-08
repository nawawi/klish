/** @file kentry.h
 *
 * @brief Klish scheme's "ENTRY" entry
 */

#ifndef _klish_kentry_h
#define _klish_kentry_h

#include <faux/list.h>
#include <klish/kaction.h>
#include <klish/khotkey.h>

typedef struct kentry_s kentry_t;

typedef faux_list_node_t kentry_entrys_node_t;
typedef faux_list_node_t kentry_actions_node_t;
typedef faux_list_node_t kentry_hotkeys_node_t;

// Mode of nested entrys list
typedef enum {
	KENTRY_MODE_NONE, // Illegal
	KENTRY_MODE_SEQUENCE, // Sequence of entrys
	KENTRY_MODE_SWITCH, // Switch of entrys
	KENTRY_MODE_EMPTY, // Entry must not have a nested entrys
} kentry_mode_e;

// Purpose of ENTRY
typedef enum {
	KENTRY_PURPOSE_NONE,
	KENTRY_PURPOSE_COMMON, // Common entry like view/command/param
	KENTRY_PURPOSE_PTYPE,
	KENTRY_PURPOSE_PROMPT, // Prompt for view
	KENTRY_PURPOSE_COND, // Conditional expression
	KENTRY_PURPOSE_COMPLETION, // Engine to generate completions
	KENTRY_PURPOSE_HELP,
	KENTRY_PURPOSE_LOG, // Log klish commands
	KENTRY_PURPOSE_MAX,
} kentry_purpose_e;

// Filter flag
typedef enum {
	KENTRY_FILTER_NONE, // Illegal
	KENTRY_FILTER_FALSE, // Entry is not a filter
	KENTRY_FILTER_TRUE, // Entry is a filter
	KENTRY_FILTER_DUAL, // Entry can be filter or non-filter
} kentry_filter_e;

// Number of max occurs
typedef enum {
	KENTRY_OCCURS_UNBOUNDED = (size_t)(-1),
} kentry_occurs_e;


typedef bool_t (*kentry_udata_free_fn)(void *data);


C_DECL_BEGIN

kentry_t *kentry_new(const char *name);
void kentry_free(kentry_t *entry);

bool_t kentry_link(kentry_t *dst, const kentry_t *src);

// Name
const char *kentry_name(const kentry_t *entry);
// Help
const char *kentry_help(const kentry_t *entry);
bool_t kentry_set_help(kentry_t *entry, const char *help);
// Parent
kentry_t *kentry_parent(const kentry_t *entry);
bool_t kentry_set_parent(kentry_t *entry, kentry_t *parent);
// Container
bool_t kentry_container(const kentry_t *entry);
bool_t kentry_set_container(kentry_t *entry, bool_t container);
// Mode
kentry_mode_e kentry_mode(const kentry_t *entry);
bool_t kentry_set_mode(kentry_t *entry, kentry_mode_e mode);
// Purpose
kentry_purpose_e kentry_purpose(const kentry_t *entry);
bool_t kentry_set_purpose(kentry_t *entry, kentry_purpose_e purpose);
// Min occurs
size_t kentry_min(const kentry_t *entry);
bool_t kentry_set_min(kentry_t *entry, size_t min);
// Max occurs
size_t kentry_max(const kentry_t *entry);
bool_t kentry_set_max(kentry_t *entry, size_t max);
// Ref
const char *kentry_ref_str(const kentry_t *entry);
bool_t kentry_set_ref_str(kentry_t *entry, const char *ref_str);
// Value
const char *kentry_value(const kentry_t *entry);
bool_t kentry_set_value(kentry_t *entry, const char *value);
// Restore
bool_t kentry_restore(const kentry_t *entry);
bool_t kentry_set_restore(kentry_t *entry, bool_t restore);
// Order
bool_t kentry_order(const kentry_t *entry);
bool_t kentry_set_order(kentry_t *entry, bool_t order);
// Filter
kentry_filter_e kentry_filter(const kentry_t *entry);
bool_t kentry_set_filter(kentry_t *entry, kentry_filter_e filter);
// User data
void *kentry_udata(const kentry_t *entry);
bool_t kentry_set_udata(kentry_t *entry, void *data, kentry_udata_free_fn udata_free_fn);

// Nested ENTRY list
faux_list_t *kentry_entrys(const kentry_t *entry);
bool_t kentry_add_entrys(kentry_t *entry, kentry_t *nested_entry);
kentry_t *kentry_find_entry(const kentry_t *entry, const char *name);
ssize_t kentry_entrys_len(const kentry_t *entry);
bool_t kentry_entrys_is_empty(const kentry_t *entry);
kentry_entrys_node_t *kentry_entrys_iter(const kentry_t *entry);
kentry_t *kentry_entrys_each(kentry_entrys_node_t **iter);

// ACTIONs
faux_list_t *kentry_actions(const kentry_t *entry);
bool_t kentry_add_actions(kentry_t *entry, kaction_t *action);
ssize_t kentry_actions_len(const kentry_t *entry);
kentry_actions_node_t *kentry_actions_iter(const kentry_t *entry);
kaction_t *kentry_actions_each(kentry_actions_node_t **iter);
kaction_io_e kentry_in(const kentry_t *entry);
kaction_io_e kentry_out(const kentry_t *entry);

// HOTKEYs
faux_list_t *kentry_hotkeys(const kentry_t *entry);
bool_t kentry_add_hotkeys(kentry_t *entry, khotkey_t *hotkey);
ssize_t kentry_hotkeys_len(const kentry_t *entry);
kentry_hotkeys_node_t *kentry_hotkeys_iter(const kentry_t *entry);
khotkey_t *kentry_hotkeys_each(kentry_hotkeys_node_t **iter);
int kentry_hotkey_compare(const void *first, const void *second);

// Fast access for nested entries with special purposes
kentry_t *kentry_nested_by_purpose(const kentry_t *entry, kentry_purpose_e purpose);
bool_t kentry_set_nested_by_purpose(kentry_t *entry, kentry_purpose_e purpose,
	kentry_t *nested);

C_DECL_END

#endif // _klish_kentry_h
