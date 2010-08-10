/*
 * shell_find_create_view.c
 */
#include "private.h"

#include <assert.h>
/*--------------------------------------------------------- */
clish_view_t *clish_shell_find_create_view(clish_shell_t * this,
					   const char *name, const char *prompt)
{
	clish_view_t *view = lub_bintree_find(&this->view_tree, name);

	if (NULL == view) {
		/* create a view */
		view = clish_view_new(name, prompt);
		assert(view);
		clish_shell_insert_view(this, view);
	} else {
		if (prompt) {
			/* set the prompt */
			clish_view__set_prompt(view, prompt);
		}
	}
	return view;
}

/*--------------------------------------------------------- */
