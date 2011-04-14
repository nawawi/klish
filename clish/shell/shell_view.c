/*
 * shell_find_create_view.c
 */

#include <assert.h>

#include "private.h"

/*--------------------------------------------------------- */
clish_view_t *clish_shell_find_create_view(clish_shell_t * this,
	const char *name, const char *prompt)
{
	clish_view_t *view = lub_bintree_find(&this->view_tree, name);

	if (NULL == view) {
		/* create a view */
		view = clish_view_new(name, prompt, clish_shell_expand);
		assert(view);
		clish_shell_insert_view(this, view);
	} else {
		/* set the prompt */
		if (prompt)
			clish_view__set_prompt(view, prompt);
	}
	return view;
}

/*--------------------------------------------------------- */
clish_view_t *clish_shell_find_view(clish_shell_t * this, const char *name)
{
	return lub_bintree_find(&this->view_tree, name);
}

/*--------------------------------------------------------- */
void clish_shell_insert_view(clish_shell_t * this, clish_view_t * view)
{
	(void)lub_bintree_insert(&this->view_tree, view);
}

/*--------------------------------------------------------- */
const clish_view_t *clish_shell__get_view(const clish_shell_t * this)
{
	assert(this);
	return this->view;
}

/*--------------------------------------------------------- */
unsigned clish_shell__get_depth(const clish_shell_t * this)
{
	assert(this);
	return clish_view__get_depth(this->view);
}

/*--------------------------------------------------------- */
