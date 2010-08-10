/*
 * shell_new.c
 */
#include "private.h"
#include "lub/string.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <unistd.h>
#include <pthread.h>
#include <dirent.h>

/*
 * if CLISH_PATH is unset in the environment then this is the value used. 
 */
const char *default_path = "/etc/clish;~/.clish";

/*-------------------------------------------------------- */
/* 
 * The context structure is used to simplify the cleanup of 
 * a CLI session when a thread is cancelled.
 */
typedef struct _context context_t;
struct _context {
	pthread_t pthread;
	const clish_shell_hooks_t *hooks;
	void *cookie;
	FILE *istream;
	clish_shell_t *shell;
	clish_pargv_t *pargv;
	char *prompt;
};
/*-------------------------------------------------------- */
/* perform a simple tilde substitution for the home directory
 * defined in HOME
 */
static char *clish_shell_tilde_expand(const char *path)
{
	char *home_dir = getenv("HOME");
	char *result = NULL;
	const char *p = path;
	const char *segment = path;
	int count = 0;
	while (*p) {
		if ('~' == *p) {
			if (count) {
				lub_string_catn(&result, segment, count);
				segment += (count + 1);	/* skip the tilde in the path */
				count = -1;
			}
			lub_string_cat(&result, home_dir);
		}
		count++;
		p++;
	}
	if (count) {
		lub_string_catn(&result, segment, count);
	}
	return result;
}

/*-------------------------------------------------------- */
void clish_shell_load_files(clish_shell_t * this)
{
	const char *path = getenv("CLISH_PATH");
	char *buffer;
	char *dirname;

	if (NULL == path) {
		/* use the default path */
		path = default_path;
	}
	/* take a copy of the path */
	buffer = clish_shell_tilde_expand(path);

	/* now loop though each directory */
	for (dirname = strtok(buffer, ";");
	     dirname; dirname = strtok(NULL, ";")) {
		DIR *dir;
		struct dirent *entry;

		/* search this directory for any XML files */
		dir = opendir(dirname);
		if (NULL == dir) {
#ifdef DEBUG
			tinyrl_printf(this->tinyrl,
				      "*** Failed to open '%s' directory\n",
				      dirname);
#endif
			continue;
		}
		for (entry = readdir(dir); entry; entry = readdir(dir)) {
			const char *extension = strrchr(entry->d_name, '.');
			/* check the filename */
			if (NULL != extension) {
				if (0 == strcmp(".xml", extension)) {
					char *filename = NULL;

					/* build the filename */
					lub_string_cat(&filename, dirname);
					lub_string_cat(&filename, "/");
					lub_string_cat(&filename,
						       entry->d_name);

					/* load this file */
					(void)clish_shell_xml_read(this,
								   filename);

					/* release the resource */
					lub_string_free(filename);
				}
			}
		}
		/* all done for this directory */
		closedir(dir);
	}
	/* tidy up */
	lub_string_free(buffer);
#ifdef DEBUG

	clish_shell_dump(this);
#endif
}

/*-------------------------------------------------------- */
/*
 * This is invoked when the thread ends or is cancelled.
 */
static void clish_shell_cleanup(context_t * context)
{
#ifdef __vxworks
	int last_state;
	/* we need to avoid recursion issues that exit in VxWorks */
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &last_state);
#endif				/* __vxworks */

	if (context->shell) {
		/*
		 * Time to kill off the instance and terminate the thread 
		 */
		clish_shell_delete(context->shell);
		context->shell = NULL;
	}
	if (context->pargv) {
		clish_pargv_delete(context->pargv);
		context->pargv = NULL;
	}
	if (context->prompt) {
		lub_string_free(context->prompt);
		context->prompt = NULL;
	}

	/* finished with this */
	free(context);
#ifdef __vxworks
	pthread_setcancelstate(last_state, &last_state);
#endif				/* __vxworks */
}

/*-------------------------------------------------------- */
/*
 * This provides the thread of execution for a shell instance
 */
static void *clish_shell_thread(void *arg)
{
	context_t *context = arg;
	bool_t running = BOOL_TRUE;
	clish_shell_t *this;
	int last_type;

	/* make sure we can only be cancelled at controlled points */
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &last_type);
	/* register a cancellation handler */
	pthread_cleanup_push((void (*)(void *))clish_shell_cleanup, context);

	/* create a shell object... */
	this = context->shell = clish_shell_new(context->hooks,
						context->cookie,
						context->istream);
	assert(this);

	/*
	 * Check the shell isn't closing down
	 */
	if (this && (SHELL_STATE_CLOSING != this->state)) {
		/*
		 * load the XML files found in the current CLISH path 
		 */
		clish_shell_load_files(this);

		/* start off with the default inputs stream */
		(void)clish_shell_push_file(this,
					    fdopen(fileno(context->istream),
						   "r"), BOOL_TRUE);

		/* This is the starting point for a shell, if this fails then 
		 * we cannot start the shell...
		 */
		running = clish_shell_startup(this);

		pthread_testcancel();

		/* Loop reading and executing lines until the user quits. */
		while (running) {
			const clish_command_t *cmd;
			const clish_view_t *view;

			if ((SHELL_STATE_SCRIPT_ERROR == this->state) &&
			    (BOOL_TRUE == tinyrl__get_isatty(this->tinyrl))) {
				/* interactive session doesn't automatically exit on error */
				this->state = SHELL_STATE_READY;
			}
			/* only bother to read the next line if there hasn't been a script error */
			if (this->state != SHELL_STATE_SCRIPT_ERROR) {
				/* obtain the prompt */
				view = clish_shell__get_view(this);
				assert(view);

				context->prompt =
				    clish_view__get_prompt(view,
							   clish_shell__get_viewid
							   (this));
				assert(context->prompt);

				/* get input from the user */
				running =
				    clish_shell_readline(this, context->prompt,
							 &cmd, &context->pargv);
				lub_string_free(context->prompt);

				context->prompt = NULL;

				if (running && cmd && context->pargv) {
					/* execute the provided command */
					if (BOOL_FALSE ==
					    clish_shell_execute(this, cmd,
								&context->
								pargv)) {
						/* there was an error */
						tinyrl_ding(this->tinyrl);

						/* 
						 * what we do now depends on whether we are set up to
						 * stop on error on not.
						 */
						if ((BOOL_TRUE ==
						     this->current_file->
						     stop_on_error)
						    && (BOOL_FALSE ==
							tinyrl__get_isatty
							(this->tinyrl))) {
							this->state =
							    SHELL_STATE_SCRIPT_ERROR;
						}
					}
				}
			}
			if ((BOOL_FALSE == running) ||
			    (this->state == SHELL_STATE_SCRIPT_ERROR)) {
				/* we've reached the end of a file (or a script error has occured)
				 * unwind the file stack to see whether 
				 * we need to exit
				 */
				running = clish_shell_pop_file(this);
			}
			/* test for cancellation */
			pthread_testcancel();
		}
	}
	/* be a good pthread citizen */
	pthread_cleanup_pop(1);

	return (void *)BOOL_TRUE;
}

/*-------------------------------------------------------- */
static context_t *_clish_shell_spawn(const pthread_attr_t * attr,
				     const clish_shell_hooks_t * hooks,
				     void *cookie, FILE * istream)
{
	int rtn;
	context_t *context = malloc(sizeof(context_t));
	assert(context);

	if (context) {
		context->hooks = hooks;
		context->cookie = cookie;
		context->istream = istream;
		context->shell = NULL;
		context->prompt = NULL;
		context->pargv = NULL;

		/* and set it free */
		rtn = pthread_create(&context->pthread,
				     attr, clish_shell_thread, context);
		if (0 != rtn) {
			free(context);
			context = NULL;
		}
	}
	return context;
}

/*-------------------------------------------------------- */
static int
_clish_shell_spawn_and_wait(const clish_shell_hooks_t * hooks,
			    void *cookie, FILE * file)
{
	void *result = NULL;
	context_t *context = _clish_shell_spawn(NULL, hooks, cookie, file);

	if (context) {
		/* join the shell's thread and wait for it to exit */
		(void)pthread_join(context->pthread, &result);
	}
	return result ? BOOL_TRUE : BOOL_FALSE;
}

/*-------------------------------------------------------- */
int clish_shell_spawn_and_wait(const clish_shell_hooks_t * hooks, void *cookie)
{
	return _clish_shell_spawn_and_wait(hooks, cookie, stdin);
}

/*-------------------------------------------------------- */
bool_t
clish_shell_spawn(pthread_t * pthread,
		  const pthread_attr_t * attr,
		  const clish_shell_hooks_t * hooks, void *cookie)
{
	context_t *context;
	bool_t result = BOOL_FALSE;

	/* spawn the thread... */
	context = _clish_shell_spawn(attr, hooks, cookie, stdin);

	if (NULL != context) {
		result = BOOL_TRUE;
		if (NULL != pthread) {
			*pthread = context->pthread;
		}
	}
	return result;
}

/*-------------------------------------------------------- */
bool_t
clish_shell_spawn_from_file(const clish_shell_hooks_t * hooks,
			    void *cookie, const char *filename)
{
	bool_t result = BOOL_FALSE;
	if (NULL != filename) {
		FILE *file = fopen(filename, "r");
		if (NULL != file) {
			/* spawn the thread and wait for it to exit */
			result =
			    _clish_shell_spawn_and_wait(hooks, cookie,
							file) ? BOOL_TRUE :
			    BOOL_FALSE;

			fclose(file);
		}
	}
	return result;
}

/*-------------------------------------------------------- */
