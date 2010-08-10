/*
 * shell_tinyrl.c
 * 
 * This is a specialisation of the tinyrl_t class which maps the readline
 * functionality to the CLISH envrionment.
 */
#include "private.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "tinyrl/tinyrl.h"
#include "tinyrl/history.h"

#include "lub/string.h"

/* This is used to hold context during tinyrl callbacks */
typedef struct _context context_t;
struct _context {
	clish_shell_t *shell;
	const clish_command_t *command;
	clish_pargv_t *pargv;
};
/*-------------------------------------------------------- */
static bool_t clish_shell_tinyrl_key_help(tinyrl_t * this, int key)
{
	bool_t result = BOOL_TRUE;
	if (BOOL_TRUE == tinyrl_is_quoting(this)) {
		/* if we are in the middle of a quote then simply enter a space */
		result = tinyrl_insert_text(this, "?");
	} else {
		/* get the context */
		context_t *context = tinyrl__get_context(this);

		tinyrl_crlf(this);
		tinyrl_crlf(this);
		clish_shell_help(context->shell, tinyrl__get_line(this));
		tinyrl_crlf(this);
		tinyrl_reset_line_state(this);
	}
	/* keep the compiler happy */
	key = key;

	return result;
}

/*-------------------------------------------------------- */
/* Generator function for command completion.  STATE lets us
 * know whether to start from scratch; without any state
 *  (i.e. STATE == 0), then we start at the top of the list. 
 */
/*lint -e818
  Pointer paramter 'this' could be declared as pointing to const */
static char *clish_shell_tinyrl_word_generator(tinyrl_t * this,
					       const char *line,
					       unsigned offset, unsigned state)
{
	/* get the context */
	context_t *context = tinyrl__get_context(this);

	return clish_shell_word_generator(context->shell, line, offset, state);
}

/*lint +e818 */
/*-------------------------------------------------------- */
/*
 * Expand the current line with any history substitutions
 */
static clish_pargv_status_t clish_shell_tinyrl_expand(tinyrl_t * this)
{
	clish_pargv_status_t status = clish_LINE_OK;
	int rtn;
	char *buffer;

	/* first of all perform any history substitutions */
	rtn = tinyrl_history_expand(tinyrl__get_history(this),
				    tinyrl__get_line(this), &buffer);

	switch (rtn) {
	case -1:
		/* error in expansion */
		status = clish_BAD_HISTORY;
		break;
	case 0:
		/*no expansion */
		break;
	case 1:
		/* expansion occured correctly */
		tinyrl_replace_line(this, buffer, 1);
		break;
	case 2:
		/* just display line */
		printf("\n%s", buffer);
		free(buffer);
		buffer = NULL;
		break;
	default:
		break;
	}
	free(buffer);

	return status;
}

/*-------------------------------------------------------- */
/*
 * This is a CLISH specific completion function.
 * If the current prefix is not a recognised prefix then
 * an error is flagged.
 * If it is a recognisable prefix then possible completions are displayed
 * or a unique completion is inserted.
 */
static tinyrl_match_e clish_shell_tinyrl_complete(tinyrl_t * this)
{
	context_t *context = tinyrl__get_context(this);
	tinyrl_match_e status;

	/* first of all perform any history expansion */
	(void)clish_shell_tinyrl_expand(this);

	/* perform normal completion */
	status = tinyrl_complete(this);

	switch (status) {
	case TINYRL_NO_MATCH:
		{
			if (BOOL_FALSE == tinyrl_is_completion_error_over(this)) {
				/* The user hasn't even entered a valid prefix!!! */
//				tinyrl_crlf(this);
//				clish_shell_help(context->shell,
//						 tinyrl__get_line(this));
//				tinyrl_crlf(this);
//				tinyrl_reset_line_state(this);
			}
			break;
		}
	case TINYRL_MATCH:
	case TINYRL_MATCH_WITH_EXTENSIONS:
	case TINYRL_COMPLETED_MATCH:
	case TINYRL_AMBIGUOUS:
	case TINYRL_COMPLETED_AMBIGUOUS:
		{
			/* the default completion function will have prompted for completions as
			 * necessary
			 */
			break;
		}
	}
	return status;
}

/*--------------------------------------------------------- */
static bool_t clish_shell_tinyrl_key_space(tinyrl_t * this, int key)
{
	bool_t result = BOOL_FALSE;
	tinyrl_match_e status;

	if (BOOL_TRUE == tinyrl_is_quoting(this)) {
		/* if we are in the middle of a quote then simply enter a space */
		result = tinyrl_insert_text(this, " ");
	} else {
		/* perform word completion */
		status = clish_shell_tinyrl_complete(this);

		switch (status) {
		case TINYRL_NO_MATCH:
		case TINYRL_AMBIGUOUS:
			{
				/* ambiguous result signal an issue */
				break;
			}
		case TINYRL_COMPLETED_AMBIGUOUS:
			{
				/* perform word completion again in case we just did case
				   modification the first time */
				status = clish_shell_tinyrl_complete(this);
				if (status == TINYRL_MATCH_WITH_EXTENSIONS) {
					/* all is well with the world just enter a space */
					result = tinyrl_insert_text(this, " ");
				}
				break;
			}
		case TINYRL_MATCH:
		case TINYRL_MATCH_WITH_EXTENSIONS:
		case TINYRL_COMPLETED_MATCH:
			{
				/* all is well with the world just enter a space */
				result = tinyrl_insert_text(this, " ");
				break;
			}
		}
	}
	/* keep compiler happy */
	key = key;

	return result;
}

/*-------------------------------------------------------- */
static bool_t clish_shell_tinyrl_key_enter(tinyrl_t * this, int key)
{
	context_t *context = tinyrl__get_context(this);
	const clish_command_t *cmd = NULL;
	const char *line = tinyrl__get_line(this);
	bool_t result = BOOL_FALSE;

	if (*line) {
		/* try and parse the command */
		cmd = clish_shell_resolve_command(context->shell, line);
		if (NULL == cmd) {
			tinyrl_match_e status =
			    clish_shell_tinyrl_complete(this);
			switch (status) {
			case TINYRL_NO_MATCH:
			case TINYRL_AMBIGUOUS:
			case TINYRL_COMPLETED_AMBIGUOUS:
				{
					/* failed to get a unique match... */
					break;
				}
			case TINYRL_MATCH:
			case TINYRL_MATCH_WITH_EXTENSIONS:
			case TINYRL_COMPLETED_MATCH:
				{
					/* re-fetch the line as it may have changed
					 * due to auto-completion
					 */
					line = tinyrl__get_line(this);
					/* get the command to parse? */
					cmd =
					    clish_shell_resolve_command
					    (context->shell, line);
					if (NULL == cmd) {
						/*
						 * We have had a match but it is not a command
						 * so add a space so as not to confuse the user
						 */
						result =
						    tinyrl_insert_text(this,
								       " ");
					}
					break;
				}
			}
		}
		if (NULL != cmd) {
			clish_pargv_status_t arg_status;
			tinyrl_crlf(this);
			/* we've got a command so check the syntax */
			arg_status = clish_shell_parse(context->shell,
						       line,
						       &context->command,
						       &context->pargv);
			switch (arg_status) {
			case clish_LINE_OK:
				tinyrl_done(this);
				result = BOOL_TRUE;
				break;
			case clish_BAD_HISTORY:
			case clish_BAD_CMD:
			case clish_BAD_PARAM:
				tinyrl_crlf(this);
				printf("Error. Illegal command line.\n");
				tinyrl_crlf(this);
				tinyrl_reset_line_state(this);
				break;
			}
		}
	} else {
		/* nothing to pass simply move down the screen */
		tinyrl_crlf(this);
		tinyrl_reset_line_state(this);
		result = BOOL_TRUE;
	}
	if ((BOOL_FALSE == result) && (BOOL_FALSE == tinyrl__get_isatty(this))) {
		/* we've found an error in a script */
		context->shell->state = SHELL_STATE_SCRIPT_ERROR;
	}
	/* keep the compiler happy */
	key = key;
	return result;
}

/*-------------------------------------------------------- */
/* This is the completion function provided for CLISH */
static tinyrl_completion_func_t clish_shell_tinyrl_completion;
static char **clish_shell_tinyrl_completion(tinyrl_t * this,
					    const char *line,
					    unsigned start, unsigned end)
{
	char **matches;

	/* don't bother to resort to filename completion */
	tinyrl_completion_over(this);

	/* perform the matching */
	matches = tinyrl_completion(this,
				    line,
				    start,
				    end, clish_shell_tinyrl_word_generator);
	return matches;
}

/*-------------------------------------------------------- */
static void clish_shell_tinyrl_init(tinyrl_t * this)
{
	bool_t status;
	/* bind the '?' key to the help function */
	status = tinyrl_bind_key(this, '?', clish_shell_tinyrl_key_help);
	assert(BOOL_TRUE == status);

	/* bind the <RET> key to the help function */
	status = tinyrl_bind_key(this, '\r', clish_shell_tinyrl_key_enter);
	assert(BOOL_TRUE == status);
	status = tinyrl_bind_key(this, '\n', clish_shell_tinyrl_key_enter);
	assert(BOOL_TRUE == status);

	/* bind the <SPACE> key to auto-complete if necessary */
	status = tinyrl_bind_key(this, ' ', clish_shell_tinyrl_key_space);
	assert(BOOL_TRUE == status);
}

/*-------------------------------------------------------- */
/* 
 * Create an instance of the specialised class
 */
tinyrl_t *clish_shell_tinyrl_new(FILE * istream,
				 FILE * ostream, unsigned stifle)
{
	/* call the parent constructor */
	tinyrl_t *this = tinyrl_new(istream,
				    ostream,
				    stifle,
				    clish_shell_tinyrl_completion);
	if (NULL != this) {
		/* now call our own constructor */
		clish_shell_tinyrl_init(this);
	}
	return this;
}

/*-------------------------------------------------------- */
void clish_shell_tinyrl_fini(tinyrl_t * this)
{
	/* nothing to do... yet */
	this = this;
}

/*-------------------------------------------------------- */
void clish_shell_tinyrl_delete(tinyrl_t * this)
{
	/* call our destructor */
	clish_shell_tinyrl_fini(this);

	/* and call the parent destructor */
	tinyrl_delete(this);
}

/*-------------------------------------------------------- */

bool_t
clish_shell_readline(clish_shell_t * this,
		     const char *prompt,
		     const clish_command_t ** cmd, clish_pargv_t ** pargv)
{
	char *line = NULL;
	bool_t result = BOOL_FALSE;
	context_t context;

	/* set up the context */
	context.command = NULL;
	context.pargv = NULL;
	context.shell = this;

	if (SHELL_STATE_CLOSING != this->state) {
		this->state = SHELL_STATE_READY;

		line = tinyrl_readline(this->tinyrl, prompt, &context);
		if (NULL != line) {
			tinyrl_history_t *history =
			    tinyrl__get_history(this->tinyrl);

			if (tinyrl__get_isatty(this->tinyrl)) {
				/* deal with the history list */
				tinyrl_history_add(history, line);
			}
			if (this->client_hooks->cmd_line_fn) {
				/* now let the client know the command line has been entered */
				this->client_hooks->cmd_line_fn(this, line);
			}
			free(line);
			result = BOOL_TRUE;
			*cmd = context.command;
			*pargv = context.pargv;
		}
	}
	return result;
}

/*-------------------------------------------------------- */
