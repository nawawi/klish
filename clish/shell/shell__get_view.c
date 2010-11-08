/*
 * shell__get_view.c
 */

#include <assert.h>

#include "private.h"

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
