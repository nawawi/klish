/*
 * shell_find_create_view.c
 */

#include <assert.h>
#include <string.h>

#include "private.h"

static int find_view(const void *key, const void *data)
{
	clish_view_t *view = (clish_view_t *)data;
	const char *name = key;
	return strcmp(name, clish_view__get_name(view));
}

/*--------------------------------------------------------- */
clish_view_t *clish_shell_find_create_view(clish_shell_t *this,
	const char *name)
{
	clish_view_t *view = clish_shell_find_view(this, name);
	if (view)
		return view;
	view = clish_view_new(name);
	lub_list_add(this->view_tree, view);
	return view;
}

/*--------------------------------------------------------- */
clish_view_t *clish_shell_find_view(clish_shell_t *this, const char *name)
{
	return lub_list_find(this->view_tree, find_view, name);
}

/*--------------------------------------------------------- */
clish_view_t *clish_shell__get_view(const clish_shell_t * this)
{
	assert(this);
	if (this->depth < 0)
		return NULL;
	return this->pwdv[this->depth]->view;
}

/*--------------------------------------------------------- */
clish_view_t *clish_shell__set_depth(clish_shell_t *this, unsigned int depth)
{
	clish_view_t *tmp;

	assert(this);
	/* Check if target view is valid = is not NULL */
	tmp = this->pwdv[depth]->view;
	if (!tmp)
		return NULL;
	this->depth = depth;
	return tmp;
}

CLISH_GET(shell, unsigned int, depth);

