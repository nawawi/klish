/*
 * shell_help.c
 */
#include "private.h"
#include "clish/types.h"
#include "lub/string.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

/*--------------------------------------------------------- */
/*
 * Provide a detailed list of the possible command completions
 */
static int available_commands(clish_shell_t * this,
	clish_help_t *help, const char *line)
{
	size_t max_width = 0;
	const clish_command_t *cmd;
	clish_shell_iterator_t iter;

	/* Search for COMMAND completions */
	clish_shell_iterator_init(&iter, CLISH_NSPACE_HELP);
	while ((cmd = clish_shell_find_next_completion(this, line, &iter))) {
		size_t width;
		const char *name = clish_command__get_suffix(cmd);
		width = strlen(name);
		if (width > max_width)
			max_width = width;
		lub_argv_add(help->name, name);
		lub_argv_add(help->help, clish_command__get_text(cmd));
		lub_argv_add(help->detail, clish_command__get_detail(cmd));
	}

	return max_width;
}

/*--------------------------------------------------------- */
void clish_shell_help(clish_shell_t *this, const char *line)
{
	clish_help_t help;
	size_t max_width = 0;
	const clish_command_t *cmd;
	int i;

	help.name = lub_argv_new(NULL, 0);
	help.help = lub_argv_new(NULL, 0);
	help.detail = lub_argv_new(NULL, 0);

	/* Get COMMAND completions */
	max_width = available_commands(this, &help, line);

	/* Resolve a command */
	cmd = clish_shell_resolve_command(this, line);
	/* Search for PARAM completion */
	if (cmd) {
		size_t width = 0;
		width = clish_command_help(cmd, &help, this->viewid, line);
		if (width > max_width)
			max_width = width;
	}
	if (lub_argv__get_count(help.name) == 0)
		goto end;

	/* Print help messages */
	for (i = 0; i < lub_argv__get_count(help.name); i++) {
		fprintf(stderr, "  %-*s  %s\n", (int)max_width,
			lub_argv__get_arg(help.name, i),
			lub_argv__get_arg(help.help, i));
	}

	/* Print details */
	if ((lub_argv__get_count(help.name) == 1) &&
		(SHELL_STATE_HELPING == this->state)) {
		const char *detail = lub_argv__get_arg(help.detail, 0);
		if (detail)
			fprintf(stderr, "%s\n", detail);
	}

	/* update the state */
	if (this->state == SHELL_STATE_HELPING)
		this->state = SHELL_STATE_OK;
	else
		this->state = SHELL_STATE_HELPING;

end:
	lub_argv_delete(help.name);
	lub_argv_delete(help.help);
	lub_argv_delete(help.detail);
}

/*--------------------------------------------------------- */
