/*
 * shell_resolve_prefix.c
 */
#include "private.h"

/*--------------------------------------------------------- */
const clish_command_t *clish_shell_resolve_prefix(const clish_shell_t * this,
						  const char *line)
{
	clish_command_t *cmd, *result;

	/* Search the current view */
	result = clish_view_resolve_prefix(this->view, line, BOOL_TRUE);
	/* Search the global view */
	cmd = clish_view_resolve_prefix(this->global, line, BOOL_TRUE);

	result = clish_command_choose_longest(result, cmd);

	return result;
}

/*----------------------------------------------------------- */
