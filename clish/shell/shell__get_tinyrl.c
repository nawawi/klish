/*
 * shell__get_tinyrl.c
 */
#include "private.h"

/*--------------------------------------------------------- */
tinyrl_t *clish_shell__get_tinyrl(const clish_shell_t * this)
{
	return this->tinyrl;
}

/*--------------------------------------------------------- */
