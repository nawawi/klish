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
