/*
 * shell_word_generator.c
 */
#include <string.h>

#include "private.h"
#include "lub/string.h"
#include "lub/argv.h"
/*-------------------------------------------------------- */
void
clish_shell_iterator_init(clish_shell_iterator_t * iter,
	clish_nspace_visibility_t field)
{
	iter->last_cmd = NULL;
	iter->field = field;
}

/*-------------------------------------------------------- */
const clish_command_t *clish_shell_find_next_completion(const clish_shell_t *
	this, const char *line, clish_shell_iterator_t * iter)
{
	const clish_command_t *result, *cmd;

	/* ask the local view for next command */
	result = clish_view_find_next_completion(this->view,
		iter->last_cmd, line, iter->field, BOOL_TRUE);
	/* ask the global view for next command */
	cmd = clish_view_find_next_completion(this->global,
		iter->last_cmd, line, iter->field, BOOL_TRUE);

	if (clish_command_diff(result, cmd) > 0)
		result = cmd;

	if (!result)
		iter->last_cmd = NULL;
	else
		iter->last_cmd = clish_command__get_name(result);

	return result;
}

/*--------------------------------------------------------- */
const clish_command_t *clish_shell_getfirst_command(clish_shell_t * this,
	const char *line, clish_nspace_visibility_t field)
{
	clish_shell_iterator_init(&this->context.iter, field);

	/* find the first command for which this is a prefix */
	return clish_shell_getnext_command(this, line);
}

/*--------------------------------------------------------- */
const clish_command_t *clish_shell_getnext_command(clish_shell_t * this,
	const char *line)
{
	return clish_shell_find_next_completion(this, line, &this->context.iter);
}

/*--------------------------------------------------------- */
void clish_shell_param_generator(clish_shell_t *this, lub_argv_t *matches,
	const clish_command_t *cmd, const char *line, unsigned offset)
{
	const char *name = clish_command__get_name(cmd);
	char *text = lub_string_dup(&line[offset]);
	clish_ptype_t *ptype;
	unsigned idx = lub_argv_wordcount(name);
	/* get the index of the current parameter */
	unsigned index = lub_argv_wordcount(line) - idx;

	if ((0 != index) || (line[offset - 1] == ' ')) {
		lub_argv_t *argv = lub_argv_new(line, 0);
		clish_pargv_t *pargv = clish_pargv_create();
		clish_pargv_t *completion_pargv = clish_pargv_create();
		unsigned completion_index = 0;
		const clish_param_t *param = NULL;

		/* if there is some text for the parameter then adjust the index */
		if ((0 != index) && (text[0] != '\0'))
			index--;

		/* Parse command line to get completion pargv's */
		clish_pargv_parse(pargv, cmd, this->viewid,
			clish_command__get_paramv(cmd),
			argv, &idx, completion_pargv, index + idx);
		clish_pargv_delete(pargv);
		lub_argv_delete(argv);

		while ((param = clish_pargv__get_param(completion_pargv,
			completion_index++))) {
			char *result;
			/* The param is args so it has no completion */
			if (param == clish_command__get_args(cmd))
				continue;
			/* The switch has no completion string */
			if (CLISH_PARAM_SWITCH == clish_param__get_mode(param))
				continue;
			/* The subcommand is identified by it's value */
			if (CLISH_PARAM_SUBCOMMAND ==
				clish_param__get_mode(param)) {
				result = clish_param__get_value(param);
				if (result)
					lub_argv_add(matches, result);
			}
			/* The common PARAM. Let ptype do the work */
			if ((ptype = clish_param__get_ptype(param)))
				clish_ptype_word_generator(ptype, matches, text);
		}
		clish_pargv_delete(completion_pargv);
	}

	lub_string_free(text);
}

/*--------------------------------------------------------- */
