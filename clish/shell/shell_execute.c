/*
 * shell_execute.c
 */
#include "private.h"
#include "lub/string.h"
#include "lub/argv.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

/*
 * These are the internal commands for this framework.
 */
static clish_shell_builtin_fn_t
    clish_close,
    clish_overview,
    clish_source, clish_source_nostop, clish_history, clish_running_config;

static clish_shell_builtin_t clish_cmd_list[] = {
	{"clish_close", clish_close},
	{"clish_overview", clish_overview},
	{"clish_source", clish_source},
	{"clish_source_nostop", clish_source_nostop},
	{"clish_history", clish_history},
	{"clish_running_config", clish_running_config},
	{NULL, NULL}
};

/*----------------------------------------------------------- */
/*
 Terminate the current shell session 
*/
static bool_t clish_close(const clish_shell_t * shell, const lub_argv_t * argv)
{
	/* the exception proves the rule... */
	clish_shell_t *this = (clish_shell_t *) shell;

	argv = argv;		/* not used */
	this->state = SHELL_STATE_CLOSING;

	return BOOL_TRUE;
}

/*----------------------------------------------------------- */
/*
 Open a file and interpret it as a script in the context of a new
 thread. Whether the script continues after command, but not script, 
 errors depends on the value of the stop_on_error flag.
*/
static bool_t
clish_source_internal(const clish_shell_t * shell,
		      const lub_argv_t * argv, bool_t stop_on_error)
{
	bool_t result = BOOL_FALSE;
	const char *filename = lub_argv__get_arg(argv, 0);
	struct stat fileStat;
	FILE *file;

	/* the exception proves the rule... */
	clish_shell_t *this = (clish_shell_t *) shell;

	/*
	 * Check file specified is not a directory 
	 */
	if (0 == stat((char *)filename, &fileStat)) {
		if (!S_ISDIR(fileStat.st_mode)) {
			file = fopen(filename, "r");
			if (NULL != file) {

				/* 
				 * push this file onto the file stack associated with this
				 * session. This will be closed by clish_shell_pop_file() 
				 * when it is finished with. 
				 */
				result =
				    clish_shell_push_file((clish_shell_t *)
							  this, file,
							  stop_on_error);
				if (BOOL_FALSE == result) {
					/* close the file here */
					fclose(file);
				}
			}
		}
	}
	return result;
}

/*----------------------------------------------------------- */
/*
 Open a file and interpret it as a script in the context of a new
 thread. Invoking a script in this way will cause the script to
 stop on the first error
*/
static bool_t clish_source(const clish_shell_t * shell, const lub_argv_t * argv)
{
	return (clish_source_internal(shell, argv, BOOL_TRUE));
}

/*----------------------------------------------------------- */
/*
 Open a file and interpret it as a script in the context of a new
 thread. Invoking a script in this way will cause the script to
 continue after command, but not script, errors.
*/
static bool_t
clish_source_nostop(const clish_shell_t * shell, const lub_argv_t * argv)
{
	return (clish_source_internal(shell, argv, BOOL_FALSE));
}

/*----------------------------------------------------------- */
/*
 Show the shell overview
*/
static bool_t
clish_overview(const clish_shell_t * this, const lub_argv_t * argv)
{
	argv = argv;		/* not used */

	tinyrl_printf(this->tinyrl, "%s\n", this->overview);

	return BOOL_TRUE;
}

/*----------------------------------------------------------- */
static bool_t clish_history(const clish_shell_t * this, const lub_argv_t * argv)
{
	tinyrl_history_t *history = tinyrl__get_history(this->tinyrl);
	tinyrl_history_iterator_t iter;
	const tinyrl_history_entry_t *entry;
	unsigned limit = 0;
	const char *arg = lub_argv__get_arg(argv, 0);

	if ((NULL != arg) && ('\0' != *arg)) {
		limit = (unsigned)atoi(arg);

		if (0 == limit) {
			/* unlimit the history list */
			(void)tinyrl_history_unstifle(history);
		} else {
			/* limit the scope of the history list */
			tinyrl_history_stifle(history, limit);
		}
	}
	for (entry = tinyrl_history_getfirst(history, &iter);
	     entry; entry = tinyrl_history_getnext(&iter)) {
		/* dump the details of this entry */
		tinyrl_printf(this->tinyrl,
			      "%5d  %s\n",
			      tinyrl_history_entry__get_index(entry),
			      tinyrl_history_entry__get_line(entry));
	}
	return BOOL_TRUE;
}

/*----------------------------------------------------------- */
/*
 Write running-config to the specified file.
*/
static bool_t
clish_running_config(const clish_shell_t * shell, const lub_argv_t * argv)
{
/*    bool_t result = BOOL_FALSE;
    FILE *stream  = stdout;
    clish_conf_t *conf = clish_shell__get_conf(shell);

    if (argv && (lub_argv__get_count(argv) > 0)) {
        if (!(stream = fopen(lub_argv__get_arg(argv,0),"w")))
            return BOOL_FALSE;
        clish_conf_fprintf(stream, conf);
        fclose(stream);
    } else
        clish_conf_fprintf(stream, conf);
*/
}

/*----------------------------------------------------------- */
/*
 * Searches for a builtin command to execute
 */
static clish_shell_builtin_fn_t *find_builtin_callback(const
						       clish_shell_builtin_t *
						       cmd_list,
						       const char *name)
{
	const clish_shell_builtin_t *result;

	/* search a list of commands */
	for (result = cmd_list; result && result->name; result++) {
		if (0 == strcmp(name, result->name)) {
			break;
		}
	}
	return (result && result->name) ? result->callback : NULL;
}

/*----------------------------------------------------------- */
void clish_shell_cleanup_script(void *script)
{
	/* simply release the memory */
	lub_string_free(script);
}

/*----------------------------------------------------------- */
bool_t
clish_shell_execute(clish_shell_t * this,
		    const clish_command_t * cmd, clish_pargv_t ** pargv)
{
	bool_t result = BOOL_TRUE;
	const char *builtin;
	char *script;

	assert(NULL != cmd);

	builtin = clish_command__get_builtin(cmd);
	script = clish_command__get_action(cmd, this->viewid, *pargv);
	/* account for thread cancellation whilst running a script */
	pthread_cleanup_push((void (*)(void *))clish_shell_cleanup_script,
			     script);
	if (NULL != builtin) {
		clish_shell_builtin_fn_t *callback;
		lub_argv_t *argv = script ? lub_argv_new(script, 0) : NULL;

		result = BOOL_FALSE;

		/* search for an internal command */
		callback = find_builtin_callback(clish_cmd_list, builtin);

		if (NULL == callback) {
			/* search for a client command */
			callback =
			    find_builtin_callback(this->client_hooks->cmd_list,
						  builtin);
		}
		if (NULL != callback) {
			/* invoke the builtin callback */
			result = callback(this, argv);
		}
		if (NULL != argv) {
			lub_argv_delete(argv);
		}
	} else if (NULL != script) {
		/* now get the client to interpret the resulting script */
		result = this->client_hooks->script_fn(this, script);

	}
	pthread_cleanup_pop(1);
	if (BOOL_TRUE == result) {
		/* move into the new view */
		clish_view_t *view = clish_command__get_view(cmd);
		char *viewid =
		    clish_command__get_viewid(cmd, this->viewid, *pargv);

		/* now get the client to config operations */
		if (this->client_hooks->config_fn)
			this->client_hooks->config_fn(this, cmd, *pargv);

		if (NULL != view) {
			/* Save the current config PWD */
			char *line = clish_variable__get_line(cmd, *pargv);
			clish_shell__set_pwd(this,
					     clish_command__get_depth(cmd),
					     line);
			lub_string_free(line);
			/* Change view */
			this->view = view;
		}
		if (viewid) {
			/* cleanup */
			lub_string_free(this->viewid);
			this->viewid = viewid;
		}
	}
	if (NULL != *pargv) {
		clish_pargv_delete(*pargv);
		*pargv = NULL;
	}
	return result;
}

/*----------------------------------------------------------- */
