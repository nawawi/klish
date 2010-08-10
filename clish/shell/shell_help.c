/*
 * shell_help.c
 */
#include "private.h"
#include "lub/string.h"

#include <stdio.h>
#include <string.h>

/*--------------------------------------------------------- */
/*
 * Provide a detailed list of the possible command completions
 */
static void
available_commands(clish_shell_t * this, const char *line, bool_t full)
{
	char *buf = NULL;
	size_t max_width = 0;
	const clish_command_t *cmd;
	if (NULL == clish_shell_getfirst_command(this, line, CLISH_NSPACE_HELP)) {
		/*
		 * A totally wrong command has been inputed
		 * Indicate the point of error and display 
		 * a list of possible commands
		 */
		char *prompt = clish_view__get_prompt(this->view,
						      this->viewid);
		unsigned error_offset = strlen(prompt) + 1;
		lub_string_free(prompt);

		/* find the best match... */
		cmd = clish_shell_resolve_prefix(this, line);
		if (NULL != cmd) {
			error_offset +=
			    strlen(clish_command__get_name(cmd)) + 1;
			/* take a copy for help purposes */
			buf = lub_string_dup(clish_command__get_name(cmd));
		} else {
			/* show all possible commands */
			buf = lub_string_dup("");
		}
		/* indicate the point of error */
		printf("%*s\n", error_offset, "^");
	} else {
		/* take a copy */
		buf = lub_string_dup(line);
	}
	/* iterate round to determine max_width */
	for (cmd = clish_shell_getfirst_command(this, buf, CLISH_NSPACE_HELP);
	     cmd; cmd = clish_shell_getnext_command(this, buf)) {
		size_t width;
		const char *name;
		if (full) {
			name = clish_command__get_name(cmd);
		} else {
			name = clish_command__get_suffix(cmd);
		}
		width = strlen(name);
		if (width > max_width) {
			max_width = width;
		}
	}

	/* now iterate round to print the help */
	for (cmd = clish_shell_getfirst_command(this, buf, CLISH_NSPACE_HELP);
	     cmd; cmd = clish_shell_getnext_command(this, buf)) {
		const char *name;
		if (full) {
			name = clish_command__get_name(cmd);
		} else {
			name = clish_command__get_suffix(cmd);
		}
		printf("%-*s  %s\n",
		       (int)max_width, name, clish_command__get_text(cmd));
	}
	/* cleanup */
	lub_string_free(buf);
}

/*--------------------------------------------------------- */
void clish_shell_help(clish_shell_t * this, const char *line)
{
	const clish_command_t *cmd, *next_cmd, *first_cmd;

	/* if there are further commands then we need to show them too */
	cmd = clish_shell_resolve_prefix(this, line);
	if (NULL != cmd) {
		clish_shell_iterator_t iter;

		/* skip the command already known about */
		clish_shell_iterator_init(&iter, CLISH_NSPACE_HELP);

		first_cmd = clish_shell_find_next_completion(this, line, &iter);
		next_cmd = clish_shell_find_next_completion(this, line, &iter);
	} else {
		first_cmd = next_cmd = NULL;
	}
	if ((NULL != cmd) && (NULL == next_cmd)
	    && (!first_cmd || (first_cmd == cmd))) {
		/* we've resolved a particular command */
		switch (this->state) {
		case SHELL_STATE_HELPING:
			{
				const char *detail =
				    clish_command__get_detail(cmd);
				if (NULL != detail) {
					printf("%s\n", detail);
				} else {
					/* get the command to describe itself */
					clish_command_help(cmd, line);
				}
				break;
			}
		case SHELL_STATE_READY:
		case SHELL_STATE_SCRIPT_ERROR:
			/* get the command to provide help */
			clish_command_help(cmd, line);
			break;
		case SHELL_STATE_INITIALISING:
		case SHELL_STATE_CLOSING:
			/* do nothing */
			break;
		}
	} else {
		/* dump the available commands */
		available_commands(this, line, BOOL_FALSE);
	}
	/* update the state */
	if (this->state == SHELL_STATE_HELPING) {
		this->state = SHELL_STATE_READY;
	} else {
		this->state = SHELL_STATE_HELPING;
	}
}

/*--------------------------------------------------------- */
