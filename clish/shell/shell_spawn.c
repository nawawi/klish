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
	bool_t running = BOOL_TRUE;
	clish_shell_t *this = arg;
	int last_type;

	/* make sure we can only be cancelled at controlled points */
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &last_type);
	/* register a cancellation handler */
	pthread_cleanup_push((void (*)(void *))clish_shell_thread_cleanup, this);

	/*
	 * Check the shell isn't closing down
	 */
	if (this && (SHELL_STATE_CLOSING != this->state)) {
		/* start off with the default inputs stream */
		(void)clish_shell_push_file(this,
			fdopen(fileno(tinyrl__get_istream(this->tinyrl)),"r"), BOOL_TRUE);

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
				/* get input from the user */
				running = clish_shell_readline(this);
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
int clish_shell_spawn(clish_shell_t * this,
	const pthread_attr_t * attr)
{
	if (!this)
		return -1;

	return pthread_create(&this->pthread,
		attr, clish_shell_thread, this);
}

/*-------------------------------------------------------- */
int clish_shell_wait(clish_shell_t * this)
{
	void *result = NULL;

	if (!this)
		return BOOL_FALSE;
	(void)pthread_join(this->pthread, &result);

	return result ? BOOL_TRUE : BOOL_FALSE;
}

/*-------------------------------------------------------- */
int clish_shell_spawn_and_wait(clish_shell_t * this,
	const pthread_attr_t * attr)
{
	if (clish_shell_spawn(this, attr) < 0)
		return -1;
	return clish_shell_wait(this);
}

/*-------------------------------------------------------- */
bool_t clish_shell_spawn_from_file(clish_shell_t * this,
	const pthread_attr_t * attr, const char *filename)
{
	bool_t result = BOOL_FALSE;
	FILE *file;

	if (!this || !filename)
		return result;

	file = fopen(filename, "r");
	if (NULL == file)
		return result;
	tinyrl__set_istream(this->tinyrl, file);
	/* spawn the thread and wait for it to exit */
	result = clish_shell_spawn_and_wait(this, attr) ?
		BOOL_TRUE : BOOL_FALSE;
	fclose(file);

	return result;
}

/*-------------------------------------------------------- */
