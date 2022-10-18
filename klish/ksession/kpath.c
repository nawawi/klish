/** @file kpath.c
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <faux/list.h>
#include <klish/khelper.h>
#include <klish/kpath.h>

struct kpath_s {
	faux_list_t *levels;
};


kpath_t *kpath_new()
{
	kpath_t *path = NULL;

	path = faux_zmalloc(sizeof(*path));
	assert(path);
	if (!path)
		return NULL;

	// User data blobs list
	path->levels = faux_list_new(FAUX_LIST_UNSORTED, FAUX_LIST_NONUNIQUE,
		NULL, NULL, (void (*)(void *))klevel_free);
	assert(path->levels);

	return path;
}


void kpath_free(kpath_t *path)
{
	if (!path)
		return;

	faux_list_free(path->levels);

	free(path);
}


size_t kpath_len(const kpath_t *path)
{
	assert(path);
	if (!path)
		return 0;

	return faux_list_len(path->levels);
}


size_t kpath_is_empty(const kpath_t *path)
{
	assert(path);
	if (!path)
		return 0;

	return faux_list_is_empty(path->levels);
}


bool_t kpath_push(kpath_t *path, klevel_t *level)
{
	assert(path);
	assert(level);
	if (!path)
		return BOOL_FALSE;
	if (!level)
		return BOOL_FALSE;

	if (!faux_list_add(path->levels, level))
		return BOOL_FALSE;

	return BOOL_TRUE;
}


bool_t kpath_pop(kpath_t *path)
{
	assert(path);
	if (!path)
		return BOOL_FALSE;
	if (kpath_is_empty(path))
		return BOOL_FALSE;

	return faux_list_del(path->levels, faux_list_tail(path->levels));
}


klevel_t *kpath_current(const kpath_t *path)
{
	assert(path);
	if (!path)
		return NULL;
	if (kpath_is_empty(path))
		return NULL;

	return (klevel_t *)faux_list_data(faux_list_tail(path->levels));
}


kpath_levels_node_t *kpath_iterr(const kpath_t *path)
{
	assert(path);
	if (!path)
		return NULL;
	return (kpath_levels_node_t *)faux_list_tail(path->levels);
}


klevel_t *kpath_eachr(kpath_levels_node_t **iterr)
{
	return (klevel_t *)faux_list_eachr((faux_list_node_t **)iterr);
}


kpath_levels_node_t *kpath_iter(const kpath_t *path)
{
	assert(path);
	if (!path)
		return NULL;
	return (kpath_levels_node_t *)faux_list_head(path->levels);
}


klevel_t *kpath_each(kpath_levels_node_t **iter)
{
	return (klevel_t *)faux_list_each((faux_list_node_t **)iter);
}


kpath_t *kpath_clone(const kpath_t *path)
{
	kpath_t *new_path = NULL;
	kpath_levels_node_t *iter = NULL;
	klevel_t *level = NULL;

	assert(path);
	if (!path)
		return NULL;

	new_path = kpath_new();
	iter = kpath_iter(path);
	while ((level = kpath_each(&iter)))
		kpath_push(new_path, klevel_clone(level));

	return new_path;
}


bool_t kpath_is_equal(const kpath_t *f, const kpath_t *s)
{
	kpath_levels_node_t *iter_f = NULL;
	kpath_levels_node_t *iter_s = NULL;
	klevel_t *level_f = NULL;
	klevel_t *level_s = NULL;

	if (!f && !s)
		return BOOL_TRUE;
	if (!f || !s)
		return BOOL_FALSE;

	iter_f = kpath_iterr(f);
	iter_s = kpath_iterr(s);
	do {
		level_f = kpath_eachr(&iter_f);
		level_s = kpath_eachr(&iter_s);
		if (!klevel_is_equal(level_f, level_s))
			return BOOL_FALSE;
	} while(level_f);

	return BOOL_TRUE;
}
