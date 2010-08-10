/*
 * shell_getnext_command.c
 */
#include "private.h"

/*--------------------------------------------------------- */
const clish_command_t *clish_shell_getnext_command(clish_shell_t * this,
						   const char *line)
{
	return clish_shell_find_next_completion(this, line, &this->iter);
}

/*--------------------------------------------------------- */
