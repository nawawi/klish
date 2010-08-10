/*
 * shell_set_context.c
 */
#include "private.h"

#include <assert.h>
/*--------------------------------------------------------- */
void clish_shell_set_context(clish_shell_t * this, const char *viewname)
{
	this->view = clish_shell_find_view(this, viewname);
	assert(this->view);
	assert(this->global);
}

/*--------------------------------------------------------- */
