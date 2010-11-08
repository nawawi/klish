/*
 * shell__get_viewid.c
 */

#include <assert.h>

#include "private.h"

/*--------------------------------------------------------- */
const char *clish_shell__get_viewid(const clish_shell_t * this)
{
	assert(this);
	return this->viewid;
}

/*--------------------------------------------------------- */
