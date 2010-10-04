/*
 * shell_parse.c
 */
#include "private.h"
#include "lub/string.h"

#include <string.h>
/*----------------------------------------------------------- */
clish_pargv_status_t clish_shell_parse(
	const clish_shell_t * this, const char *line,
	const clish_command_t ** cmd, clish_pargv_t ** pargv)
{
 	clish_pargv_status_t result = CLISH_BAD_CMD;

	*cmd = clish_shell_resolve_command(this, line);
	/* Now construct the parameters for the command */
	if (NULL != *cmd)
		*pargv = clish_pargv_new(*cmd, line, 0, &result);
	if (*pargv) {
		char str[100];
		char * tmp;
		int depth = clish_shell__get_depth(this);
		snprintf(str, sizeof(str) - 1, "%u", depth);
		clish_pargv_insert(*pargv, this->param_depth, str);
		tmp = clish_shell__get_pwd_full(this, depth);
		if (tmp) {
			clish_pargv_insert(*pargv, this->param_pwd, tmp);
			lub_string_free(tmp);
		}
	}

	return result;
}

/*----------------------------------------------------------- */
