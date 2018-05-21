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

	if (!view) {
		/* create a view */
		view = clish_view_new(name, prompt);
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

