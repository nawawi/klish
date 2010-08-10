/*
 * shell_find_view.c
 */
#include "private.h"

/*--------------------------------------------------------- */
clish_view_t *clish_shell_find_view(clish_shell_t * this, const char *name)
{
	return lub_bintree_find(&this->view_tree, name);
}

/*--------------------------------------------------------- */
