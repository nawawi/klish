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
/* perform a simple tilde substitution for the home directory
 * defined in HOME
 */
static char *clish_shell_tilde_expand(const char *path)
{
	char *home_dir = getenv("HOME");
	char *result = NULL;
	char *tilde;

	while ((tilde = strchr(path, '~'))) {
		lub_string_catn(&result, path, tilde - path);
		lub_string_cat(&result, home_dir);
		path = tilde + 1;
	}
	lub_string_cat(&result, path);

	return result;
}

/*-------------------------------------------------------- */
void clish_shell_load_scheme(clish_shell_t * this, const char *xml_path)
{
	const char *path = xml_path;
	char *buffer;
	char *dirname;
	char *saveptr;

	/* use the default path */
	if (!path)
		path = default_path;
	/* take a copy of the path */
	buffer = clish_shell_tilde_expand(path);

	/* now loop though each directory */
	for (dirname = strtok_r(buffer, ";", &saveptr);
		dirname; dirname = strtok_r(NULL, ";", &saveptr)) {
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
			if ((NULL != extension) &&
				(0 == strcmp(".xml", extension))) {
				char *filename = NULL;

				/* build the filename */
				lub_string_cat(&filename, dirname);
				lub_string_cat(&filename, "/");
				lub_string_cat(&filename, entry->d_name);

				/* load this file */
				(void)clish_shell_xml_read(this, filename);

				/* release the resource */
				lub_string_free(filename);
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
static int _loop(clish_shell_t * this, bool_t is_thread)
{
	int running = 0;
	int retval = SHELL_STATE_OK;

	assert(this);
	if (!tinyrl__get_istream(this->tinyrl))
		return SHELL_STATE_IO_ERROR;
	/* Check the shell isn't closing down */
	if (this && (SHELL_STATE_CLOSING == this->state))
		return retval;

	if (is_thread)
		pthread_testcancel();
	/* Loop reading and executing lines until the user quits */
	while (!running) {
		retval = SHELL_STATE_OK;
		/* Get input from the stream */
		running = clish_shell_readline(this, NULL);
		if (running) {
			switch (this->state) {
			case SHELL_STATE_SCRIPT_ERROR:
			case SHELL_STATE_SYNTAX_ERROR:
				/* Interactive session doesn't exit on error */
				if (tinyrl__get_isatty(this->tinyrl) ||
					(this->current_file &&
					!this->current_file->stop_on_error))
					running = 0;
				retval = this->state;
			default:
				break;
			}
		}
		if (SHELL_STATE_CLOSING == this->state)
			running = -1;
		if (running)
			running = clish_shell_pop_file(this);
		/* test for cancellation */
		if (is_thread)
			pthread_testcancel();
	}

	return retval;
}

/*-------------------------------------------------------- */
/*
 * This is invoked when the thread ends or is cancelled.
 */
static void clish_shell_thread_cleanup(clish_shell_t * this)
{
#ifdef __vxworks
	int last_state;
	/* we need to avoid recursion issues that exit in VxWorks */
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &last_state);
#endif				/* __vxworks */

/* Nothing to do now. The context will be free later. */

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
	clish_shell_t *this = arg;
	int last_type;
	int *res = NULL;

	res = malloc(sizeof(*res));
	*res = 0;
	/* make sure we can only be cancelled at controlled points */
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &last_type);
	/* register a cancellation handler */
	pthread_cleanup_push((void (*)(void *))clish_shell_thread_cleanup, this);

	if (this)
		*res = _loop(this, BOOL_TRUE);

	/* be a good pthread citizen */
	pthread_cleanup_pop(1);

	return res;
}

/*-------------------------------------------------------- */
int clish_shell_spawn(clish_shell_t *this,
	const pthread_attr_t *attr)
{
	if (!this)
		return -1;

	return pthread_create(&this->pthread,
		attr, clish_shell_thread, this);
}

/*-------------------------------------------------------- */
int clish_shell_wait(clish_shell_t *this)
{
	void *res = NULL;
	int retval = 0;

	if (!this)
		return -1;
	(void)pthread_join(this->pthread, &res);
	if (res) {
		retval = *(int *)res;
		free(res);
	}

	return retval;
}

/*-------------------------------------------------------- */
int clish_shell_spawn_and_wait(clish_shell_t *this,
	const pthread_attr_t *attr)
{
	if (clish_shell_spawn(this, attr) < 0)
		return -1;

	return clish_shell_wait(this);
}

/*-------------------------------------------------------- */
int clish_shell_loop(clish_shell_t *this)
{
	return _loop(this, BOOL_FALSE);
}

/*-------------------------------------------------------- */
