/** @file list.h
 * @brief Public interface for a bidirectional list.
 */

#ifndef _faux_list_h
#define _faux_list_h

#include <stddef.h>

#include "types.h"

typedef struct faux_list_node_s faux_list_node_t;
typedef struct faux_list_s faux_list_t;

typedef int faux_list_compare_fn(const void *first, const void *second);
typedef void faux_list_free_fn(void *data);
typedef int faux_list_match_fn(const void *key, const void *data);

C_DECL_BEGIN

// list_node_t methods
faux_list_node_t *faux_list_prev_node(const faux_list_node_t *node);
faux_list_node_t *faux_list_next_node(const faux_list_node_t *node);
void *faux_list_data(const faux_list_node_t *node);
faux_list_node_t *faux_list_each_node(faux_list_node_t **iter);
void *faux_list_each(faux_list_node_t **iter);

// list_t methods
faux_list_t *faux_list_new(faux_list_compare_fn compareFn,
	faux_list_free_fn freeFn);
void faux_list_free(faux_list_t *list);

faux_list_node_t *faux_list_head(const faux_list_t *list);
faux_list_node_t *faux_list_tail(const faux_list_t *list);
size_t faux_list_len(const faux_list_t *list);

faux_list_node_t *faux_list_add(faux_list_t *list, void *data);
faux_list_node_t *faux_list_add_uniq(faux_list_t *list, void *data);
faux_list_node_t *faux_list_find_add(faux_list_t *list, void *data);
void *faux_list_takeaway(faux_list_t *list, faux_list_node_t *node);
int faux_list_del(faux_list_t *list, faux_list_node_t *node);

faux_list_node_t *faux_list_match_node(const faux_list_t *list,
	faux_list_match_fn matchFn, const void *userkey,
	faux_list_node_t **saveptr);
void *faux_list_match(const faux_list_t *list,
	faux_list_match_fn matchFn, const void *userkey,
	faux_list_node_t **saveptr);
faux_list_node_t *faux_list_find_node(const faux_list_t *list,
	faux_list_match_fn matchFn, const void *userkey);
void *faux_list_find(const faux_list_t *list,
	faux_list_match_fn matchFn, const void *userkey);

C_DECL_END

#endif				/* _faux_list_h */

