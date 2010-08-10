/*
 * shell_insert_view.c
 */
#include "private.h"

/*--------------------------------------------------------- */
void clish_shell_insert_view(clish_shell_t * this, clish_view_t * view)
{
	(void)lub_bintree_insert(&this->view_tree, view);
}

/*--------------------------------------------------------- */
