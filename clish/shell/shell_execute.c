/*
 * shell_execute.c
 */
#include "private.h"
#include "lub/string.h"
#include "lub/argv.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <signal.h>

/*
 * These are the internal commands for this framework.
 */
static clish_shell_builtin_fn_t
    clish_close,
    clish_overview,
    clish_source,
    clish_source_nostop,
    clish_history,
    clish_nested_up,
    clish_nop;

static clish_shell_builtin_t clish_cmd_list[] = {
	{"clish_close", clish_close},
	{"clish_overview", clish_overview},
	{"clish_source", clish_source},
	{"clish_source_nostop", clish_source_nostop},
	{"clish_history", clish_history},
	{"clish_nested_up", clish_nested_up},
	{"clish_nop", clish_nop},
	{NULL, NULL}
};

/*----------------------------------------------------------- */
/* Terminate the current shell session */
static bool_t clish_close(const clish_shell_t * shell, const lub_argv_t * argv)
{
	/* the exception proves the rule... */
	clish_shell_t *this = (clish_shell_t *) shell;

	argv = argv; /* not used */
	this->state = SHELL_STATE_CLOSING;

	return BOOL_TRUE;
}

/*----------------------------------------------------------- */
/*
 Open a file and interpret it as a script in the context of a new
 thread. Whether the script continues after command, but not script, 
 errors depends on the value of the stop_on_error flag.
*/
static bool_t clish_source_internal(const clish_shell_t * shell,
	const lub_argv_t * argv, bool_t stop_on_error)
{
	bool_t result = BOOL_FALSE;
	const char *filename = lub_argv__get_arg(argv, 0);
	struct stat fileStat;

	/* the exception proves the rule... */
	clish_shell_t *this = (clish_shell_t *) shell;

	/*
	 * Check file specified is not a directory 
	 */
	if ((0 == stat((char *)filename, &fileStat)) &&
		(!S_ISDIR(fileStat.st_mode))) {
		/*
		 * push this file onto the file stack associated with this
		 * session. This will be closed by clish_shell_pop_file() 
		 * when it is finished with.
		 */
		result = clish_shell_push_file(this, filename,
			stop_on_error);
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
	argv = argv; /* not used */

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

	if (arg && ('\0' != *arg)) {
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
 * Searches for a builtin command to execute
 */
static clish_shell_builtin_fn_t *find_builtin_callback(const
	clish_shell_builtin_t * cmd_list, const char *name)
{
	const clish_shell_builtin_t *result;

	/* search a list of commands */
	for (result = cmd_list; result && result->name; result++) {
		if (0 == strcmp(name, result->name))
			break;
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
bool_t clish_shell_execute(clish_context_t *context, char **out)
{
	clish_shell_t *this = context->shell;
	const clish_command_t *cmd = context->cmd;
	clish_pargv_t *pargv = context->pargv;
	clish_action_t *action;
	bool_t result = BOOL_TRUE;
	char *lock_path = clish_shell__get_lockfile(this);
	int lock_fd = -1;
	sigset_t old_sigs;
	struct sigaction old_sigint, old_sigquit;

	assert(cmd);
	action = clish_command__get_action(cmd);

	/* Pre-change view if the command is from another depth/view */
        {
		clish_view_t *view = NULL;
		char *viewid = NULL;
		clish_view_restore_t restore = clish_command__get_restore(cmd);

		if ((CLISH_RESTORE_VIEW == restore) &&
			(clish_command__get_pview(cmd) != this->view))
			view = clish_command__get_pview(cmd);
		else if ((CLISH_RESTORE_DEPTH == restore) &&
			(clish_command__get_depth(cmd) <
			clish_view__get_depth(this->view))) {
			view = clish_shell__get_pwd_view(this,
				clish_command__get_depth(cmd));
			viewid = clish_shell__get_pwd_viewid(this,
				clish_command__get_depth(cmd));
		}

		if (view) {
			this->view = view;
			/* cleanup */
			lub_string_free(this->viewid);
			this->viewid = lub_string_dup(viewid);
		}
	}

	/* Lock the lockfile */
	if (lock_path && clish_command__get_lock(cmd)) {
		int i;
		int res;
		lock_fd = open(lock_path, O_RDONLY | O_CREAT, 00644);
		if (-1 == lock_fd) {
			fprintf(stderr, "Can't open lockfile %s.\n",
				lock_path);
			return BOOL_FALSE; /* can't open file */
		}
		for (i = 0; i < CLISH_LOCK_WAIT; i++) {
			res = flock(lock_fd, LOCK_EX | LOCK_NB);
			if (!res)
				break;
			if ((EBADF == errno) ||
				(EINVAL == errno) ||
				(ENOLCK == errno))
				break;
			if (EINTR == errno)
				continue;
			if (0 == i)
				fprintf(stderr,
					"Try to get lock. Please wait...\n");
			sleep(1);
		}
		if (res) {
			fprintf(stderr, "Can't get lock.\n");
			return BOOL_FALSE; /* can't get the lock */
		}
	}

	/* Ignore and block SIGINT and SIGQUIT */
	if (!clish_command__get_interrupt(cmd)) {
		struct sigaction sa;
		sigset_t sigs;
		sa.sa_flags = 0;
		sigemptyset(&sa.sa_mask);
		sa.sa_handler = SIG_IGN;
		sigaction(SIGINT, &sa, &old_sigint);
		sigaction(SIGQUIT, &sa, &old_sigquit);
		sigemptyset(&sigs);
		sigaddset(&sigs, SIGINT);
		sigaddset(&sigs, SIGQUIT);
		sigprocmask(SIG_BLOCK, &sigs, &old_sigs);
	}

	/* Execute ACTION */
	result = clish_shell_exec_action(action, context, out);

	/* Restore SIGINT and SIGQUIT */
	if (!clish_command__get_interrupt(cmd)) {
		sigprocmask(SIG_SETMASK, &old_sigs, NULL);
		/* Is the signals delivery guaranteed here (before
		   sigaction restore) for previously blocked and
		   pending signals? The simple test is working well.
		   I don't want to use sigtimedwait() function bacause
		   it needs a realtime extensions. The sigpending() with
		   the sleep() is not nice too. Report bug if clish will
		   get the SIGINT after non-interruptable action.
		*/
		sigaction(SIGINT, &old_sigint, NULL);
		sigaction(SIGQUIT, &old_sigquit, NULL);
	}

	/* Call config callback */
	if ((BOOL_TRUE == result) && this->client_hooks->config_fn)
		this->client_hooks->config_fn(context);

	/* Unlock the lockfile */
	if (lock_fd != -1) {
		flock(lock_fd, LOCK_UN);
		close(lock_fd);
	}

	/* Move into the new view */
	if (BOOL_TRUE == result) {
		clish_view_t *view = clish_command__get_view(cmd);
		char *viewid = clish_shell_expand(
			clish_command__get_viewid(cmd), context);
		if (view) {
			/* Save the current config PWD */
			char *line = clish_shell__get_line(cmd, pargv);
			clish_shell__set_pwd(this,
				clish_command__get_depth(cmd),
				line, this->view, this->viewid);
			lub_string_free(line);
			/* Change view */
			this->view = view;
		}
		if (viewid || view) {
			/* cleanup */
			lub_string_free(this->viewid);
			this->viewid = viewid;
		}
	}

	return result;
}

/*----------------------------------------------------------- */
bool_t clish_shell_exec_action(clish_action_t *action,
	clish_context_t *context, char **out)
{
	clish_shell_t *this = context->shell;
	bool_t result = BOOL_TRUE;
	const char *builtin;
	char *script;

	builtin = clish_action__get_builtin(action);
	script = clish_shell_expand(clish_action__get_script(action), context);
	if (builtin) {
		clish_shell_builtin_fn_t *callback;
		lub_argv_t *argv = script ? lub_argv_new(script, 0) : NULL;
		result = BOOL_FALSE;
		/* search for an internal command */
		callback = find_builtin_callback(clish_cmd_list, builtin);
		if (!callback) {
			/* search for a client command */
			callback = find_builtin_callback(
				this->client_hooks->cmd_list, builtin);
		}
		/* invoke the builtin callback */
		if (callback)
			result = callback(this, argv);
		if (argv)
			lub_argv_delete(argv);
	} else if (script) {
		/* now get the client to interpret the resulting script */
		result = this->client_hooks->script_fn(context, script, out);
	}
	lub_string_free(script);

	return result;
}

/*----------------------------------------------------------- */
/*
 * Find out the previous view in the stack and go to it
 */
static bool_t clish_nested_up(const clish_shell_t * shell, const lub_argv_t * argv)
{
	clish_shell_t *this = (clish_shell_t *) shell;
	clish_view_t *view = NULL;
	char *viewid = NULL;
	int depth = 0;

	if (!shell)
		return BOOL_FALSE;

	argv = argv; /* not used */
	depth = clish_view__get_depth(this->view);

	/* If depth=0 than exit */
	if (0 == depth) {
		this->state = SHELL_STATE_CLOSING;
		return BOOL_TRUE;
	}

	depth--;
	view = clish_shell__get_pwd_view(this, depth);
	viewid = clish_shell__get_pwd_viewid(this, depth);
	if (!view)
		return BOOL_FALSE;
	this->view = view;
	lub_string_free(this->viewid);
	this->viewid = viewid ? lub_string_dup(viewid) : NULL;

	return BOOL_TRUE;
}

/*----------------------------------------------------------- */
/*
 * Builtin: NOP function
 */
static bool_t clish_nop(const clish_shell_t * shell, const lub_argv_t * argv)
{
	return BOOL_TRUE;
}

/*----------------------------------------------------------- */
const char * clish_shell__get_fifo(clish_shell_t * this)
{
	char *name;
	int res;

	if (this->fifo_name) {
		if (0 == access(this->fifo_name, R_OK | W_OK))
			return this->fifo_name;
		unlink(this->fifo_name);
		lub_string_free(this->fifo_name);
		this->fifo_name = NULL;
	}

	do {
		char template[] = "/tmp/klish.fifo.XXXXXX";
		name = mktemp(template);
		if (name[0] == '\0')
			return NULL;
		res = mkfifo(name, 0600);
		if (res == 0)
			this->fifo_name = lub_string_dup(name);
	} while ((res < 0) && (EEXIST == errno));

	return this->fifo_name;
}

/*--------------------------------------------------------- */
void *clish_shell__get_client_cookie(const clish_shell_t * this)
{
	return this->client_cookie;
}

/*----------------------------------------------------------- */
