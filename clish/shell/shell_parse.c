/*
 * shell_parse.c
 */
#include "private.h"
#include "lub/string.h"

#include <string.h>
/*----------------------------------------------------------- */
clish_pargv_status_t
clish_shell_parse(const clish_shell_t * this,
		  const char *line,
		  const clish_command_t ** cmd, clish_pargv_t ** pargv)
{
	clish_pargv_status_t result = clish_BAD_CMD;
	size_t offset;
	char *prompt = clish_view__get_prompt(this->view, this->viewid);

	/* track the offset of each parameter on the command line */
	offset = strlen(prompt) + 1;

	/* cleanup */
	lub_string_free(prompt);

	*cmd = clish_shell_resolve_command(this, line);
	if (NULL != *cmd) {
		/*
		 * Now construct the parameters for the command
		 */
		*pargv = clish_pargv_new(*cmd, line, offset, &result);
	}
	return result;
}

/*----------------------------------------------------------- */
