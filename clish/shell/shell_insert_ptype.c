/*
 * shell_insert_view.c
 */
#include "private.h"

/*--------------------------------------------------------- */
void clish_shell_insert_ptype(clish_shell_t * this, clish_ptype_t * ptype)
{
	(void)lub_bintree_insert(&this->ptype_tree, ptype);
}

/*--------------------------------------------------------- */
