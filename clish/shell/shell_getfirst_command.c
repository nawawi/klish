/*
 * shell_getfirst_command.c
 */
#include "private.h"

/*--------------------------------------------------------- */
const clish_command_t *clish_shell_getfirst_command(clish_shell_t * this,
						    const char *line,
						    clish_nspace_visibility_t
						    field)
{
	clish_shell_iterator_init(&this->iter, field);

	/* find the first command for which this is a prefix */
	return clish_shell_getnext_command(this, line);
}

/*--------------------------------------------------------- */
