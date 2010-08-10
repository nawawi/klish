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
							this, const char *line,
							clish_shell_iterator_t *
							iter)
{
	const clish_command_t *result, *cmd;
	clish_nspace_t *nspace;
	clish_view_t *view;
	unsigned view_cnt = clish_view__get_nspace_count(this->view);
	int i;


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
static char *clish_shell_param_generator(clish_shell_t * this,
					    const clish_command_t * cmd,
					    const char *line,
					    unsigned offset, unsigned state)
{
	char *result = NULL;
	const char *name = clish_command__get_name(cmd);
	char *text = lub_string_dup(&line[offset]);
	unsigned index;
	const clish_param_t *param = NULL;
	clish_ptype_t *ptype;
	unsigned idx;

	/* get the index of the current parameter */
	index = lub_argv_wordcount(line) - lub_argv_wordcount(name);

	if ((0 != index) || (line[offset - 1] == ' ')) {
		if (0 == state) {
			lub_argv_t *argv;
			clish_pargv_t *pargv;
			unsigned i;
			
			if ((0 != index) && (text[0] != '\0')) {
				/* if there is some text for the parameter then adjust the index */
				index--;
			}
			argv = lub_argv_new(line, 0);
			idx = lub_argv_wordcount(name);
			if (this->completion_pargv) {
				clish_pargv_delete(this->completion_pargv);
				this->completion_pargv = NULL;
			}
			this->completion_pargv = clish_pargv_create();
			pargv = clish_pargv_create();
			clish_pargv_parse(pargv, cmd, clish_command__get_paramv(cmd),
				argv, &idx, this->completion_pargv, index + idx);
			clish_pargv_delete(pargv);
			lub_argv_delete(argv);
			this->completion_index = 0;
			this->completion_pindex = 0;
		}

		while ((param = clish_pargv__get_param(this->completion_pargv,
			this->completion_index++))) {

			if (param == clish_command__get_args(cmd)) {
				/* The param is args so it has no format */
				result = lub_string_dup(text);
			} else if (CLISH_PARAM_SUBCOMMAND ==
				clish_param__get_mode(param)) {
				/* The subcommand is identified by it's name */
				result = lub_string_dup(clish_param__get_name(param));
			} else if (CLISH_PARAM_SWITCH ==
				   clish_param__get_mode(param)) {
				/* The switch has no completion string */
				result = NULL;
			} else {
				/* The common param. Let ptype do the work */
				if (ptype = clish_param__get_ptype(param)) {
					result = clish_ptype_word_generator(ptype, text, 
						this->completion_pindex++);
					if (!result)
						this->completion_pindex = 0;
					else
						this->completion_index--;
				} else {
					result = NULL;
				}
			}

			if (result)
				break;
		}

	} else if (0 == state) {
		/* simply return the command name */
		result = lub_string_dup(clish_command__get_suffix(cmd));
	}

	if (!result) {
		clish_pargv_delete(this->completion_pargv);
		this->completion_pargv = NULL;
		/* make sure we reset the line state */
//		tinyrl_crlf(this->tinyrl);
//		tinyrl_reset_line_state(this->tinyrl);
//		tinyrl_completion_error_over(this->tinyrl);
	}
	lub_string_free(text);

	return result;
}

/*--------------------------------------------------------- */
static char *clish_shell_command_generator(clish_shell_t * this,
					   const char *line,
					   unsigned offset, unsigned state)
{
	char *result = NULL;
	const clish_command_t *cmd = NULL;
	if (0 == state) {
		cmd =
		    clish_shell_getfirst_command(this, line,
						 CLISH_NSPACE_COMPLETION);
	} else {
		cmd = clish_shell_getnext_command(this, line);
	}

	if (NULL != cmd) {
		result = lub_string_dup(clish_command__get_suffix(cmd));
	}
	/* keep the compiler happy */
	offset = offset;
	return result;
}

/*--------------------------------------------------------- */
char *clish_shell_word_generator(clish_shell_t * this,
				 const char *line,
				 unsigned offset, unsigned state)
{
	char *result = NULL;
	const clish_command_t *cmd, *next = NULL;

	/* try and resolve a command which is a prefix of the line */
	cmd = clish_shell_resolve_command(this, line);
	if (NULL != cmd) {
		clish_shell_iterator_t iter;
		/* see whether there is an extended extension */
		clish_shell_iterator_init(&iter, CLISH_NSPACE_COMPLETION);
		next = clish_shell_find_next_completion(this, line, &iter);
	}
	if ((NULL != cmd) && (NULL == next)) {
		/* this needs to be completed as a parameter */
		result =
		    clish_shell_param_generator(this, cmd, line, offset,
						   state);
	} else {
		/* this needs to be completed as a command */
		result =
		    clish_shell_command_generator(this, line, offset, state);
	}
	if (0 == state) {
		/* reset the state from a help perspective */
		this->state = SHELL_STATE_READY;
	}
	return result;
}

/*--------------------------------------------------------- */
