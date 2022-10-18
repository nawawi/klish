/** @file kpath.h
 *
 * @brief Klish path stack
 */

#ifndef _klish_kpath_h
#define _klish_kpath_h

#include <faux/list.h>
#include <klish/kentry.h>

typedef struct kpath_s kpath_t;
typedef struct klevel_s klevel_t;

typedef faux_list_node_t kpath_levels_node_t;


C_DECL_BEGIN

// Level

klevel_t *klevel_new(const kentry_t *entry);
void klevel_free(klevel_t *level);

const kentry_t *klevel_entry(const klevel_t *level);
klevel_t *klevel_clone(const klevel_t *level);
bool_t klevel_is_equal(const klevel_t *f, const klevel_t *s);

// Path

kpath_t *kpath_new(void);
void kpath_free(kpath_t *path);

size_t kpath_len(const kpath_t *path);
size_t kpath_is_empty(const kpath_t *path);
bool_t kpath_push(kpath_t *path, klevel_t *level);
bool_t kpath_pop(kpath_t *path);
klevel_t *kpath_current(const kpath_t *path);
kpath_levels_node_t *kpath_iterr(const kpath_t *path);
klevel_t *kpath_eachr(kpath_levels_node_t **iterr);
kpath_levels_node_t *kpath_iter(const kpath_t *path);
klevel_t *kpath_each(kpath_levels_node_t **iter);
kpath_t *kpath_clone(const kpath_t *path);
bool_t kpath_is_equal(const kpath_t *f, const kpath_t *s);


C_DECL_END

#endif // _klish_kpath_h
