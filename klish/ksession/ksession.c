/** @file ksession.c
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <faux/argv.h>
#include <faux/eloop.h>
#include <faux/buf.h>
#include <klish/khelper.h>
#include <klish/kview.h>
#include <klish/kscheme.h>
#include <klish/kpath.h>
#include <klish/kpargv.h>
#include <klish/ksession.h>
#include <klish/kexec.h>


struct ksession_s {
	const kscheme_t *scheme;
	kpath_t *path;
	bool_t done; // Indicates that session is over and must be closed
};


// Scheme
KGET(session, const kscheme_t *, scheme);

// Path
KGET(session, kpath_t *, path);

// Done
KGET_BOOL(session, done);
KSET_BOOL(session, done);


ksession_t *ksession_new(const kscheme_t *scheme, const char *start_entry)
{
	ksession_t *session = NULL;
	kentry_t *entry = NULL;
	const char *entry_to_search = NULL;
	klevel_t *level = NULL;

	assert(scheme);
	if (!scheme)
		return NULL;

	// Before real session allocation we will try to find starting entry.
	// Starting entry can be get from function argument, from STARTUP tag or
	// default name 'main' can be used. Don't create session if we can't get
	// starting entry at all. Priorities are (from higher) argument, STARTUP,
	// default name.
	if (start_entry)
		entry_to_search = start_entry;
	// STARTUP is not implemented yet
	else
		entry_to_search = KSESSION_STARTING_ENTRY;
	entry = kscheme_find_entry_by_path(scheme, entry_to_search);
	if (!entry)
		return NULL; // Can't find starting entry

	session = faux_zmalloc(sizeof(*session));
	assert(session);
	if (!session)
		return NULL;

	// Initialization
	session->scheme = scheme;
	// Create kpath_t stack
	session->path = kpath_new();
	assert(session->path);
	level = klevel_new(entry);
	assert(level);
	kpath_push(session->path, level);
	session->done = BOOL_FALSE;

	return session;
}


void ksession_free(ksession_t *session)
{
	if (!session)
		return;

	kpath_free(session->path);

	free(session);
}


static bool_t stop_loop_ev(faux_eloop_t *eloop, faux_eloop_type_e type,
	void *associated_data, void *user_data)
{
	ksession_t *session = (ksession_t *)user_data;

	if (!session)
		return BOOL_FALSE;

	ksession_set_done(session, BOOL_TRUE); // Stop the whole session

	// Happy compiler
	eloop = eloop;
	type = type;
	associated_data = associated_data;

	return BOOL_FALSE; // Stop Event Loop
}


static bool_t action_terminated_ev(faux_eloop_t *eloop, faux_eloop_type_e type,
	void *associated_data, void *user_data)
{
	int wstatus = 0;
	pid_t child_pid = -1;
	kexec_t *exec = (kexec_t *)user_data;

	if (!exec)
		return BOOL_FALSE;

	// Wait for any child process. Doesn't block.
	while ((child_pid = waitpid(-1, &wstatus, WNOHANG)) > 0)
		kexec_continue_command_execution(exec, child_pid, wstatus);

	// Check if kexec is done now
	if (kexec_done(exec))
		return BOOL_FALSE; // To break a loop

	// Happy compiler
	eloop = eloop;
	type = type;
	associated_data = associated_data;

	return BOOL_TRUE;
}


static bool_t action_stdout_ev(faux_eloop_t *eloop, faux_eloop_type_e type,
	void *associated_data, void *user_data)
{
	faux_eloop_info_fd_t *info = (faux_eloop_info_fd_t *)associated_data;
	kexec_t *exec = (kexec_t *)user_data;
	ssize_t r = -1;
	faux_buf_t *faux_buf = NULL;
	void *linear_buf = NULL;

	if (!exec)
		return BOOL_FALSE;

	faux_buf = kexec_bufout(exec);
	assert(faux_buf);

	do {
		ssize_t really_readed = 0;
		ssize_t linear_len =
			faux_buf_dwrite_lock_easy(faux_buf, &linear_buf);
		// Non-blocked read. The fd became non-blocked while
		// kexec_prepare().
		r = read(info->fd, linear_buf, linear_len);
		if (r > 0)
			really_readed = r;
		faux_buf_dwrite_unlock_easy(faux_buf, really_readed);
	} while (r > 0);

	// Happy compiler
	eloop = eloop;
	type = type;

	return BOOL_TRUE;
}


bool_t ksession_exec_locally(ksession_t *session, const char *line,
	int *retcode, faux_error_t *error)
{
	kexec_t *exec = NULL;
	faux_eloop_t *eloop = NULL;

	assert(session);
	if (!session)
		return BOOL_FALSE;

	// Parsing
	exec = ksession_parse_for_exec(session, line, error);
	if (!exec)
		return BOOL_FALSE;

	// Session status can be changed while parsing because it can execute
	// nested ksession_exec_locally() to check for PTYPEs, CONDitions etc.
	// So check for 'done' flag to propagate it.
	if (ksession_done(session)) {
		kexec_free(exec);
		return BOOL_FALSE; // Because action is not completed
	}

	// Execute kexec and then wait for completion using local Eloop
	if (!kexec_exec(exec)) {
		kexec_free(exec);
		return BOOL_FALSE; // Something went wrong
	}
	// If kexec contains only non-exec (for example dry-run) ACTIONs then
	// we don't need event loop and can return here.
	if (kexec_retcode(exec, retcode)) {
		kexec_free(exec);
		return BOOL_TRUE;
	}

	// Local service loop
	eloop = faux_eloop_new(NULL);
	faux_eloop_add_signal(eloop, SIGINT, stop_loop_ev, session);
	faux_eloop_add_signal(eloop, SIGTERM, stop_loop_ev, session);
	faux_eloop_add_signal(eloop, SIGQUIT, stop_loop_ev, session);
	faux_eloop_add_signal(eloop, SIGCHLD, action_terminated_ev, exec);
	faux_eloop_add_fd(eloop, kexec_stdout(exec), POLLIN,
		action_stdout_ev, exec);
	faux_eloop_loop(eloop);
	faux_eloop_free(eloop);

	kexec_retcode(exec, retcode);

	// Debug only
	{
		faux_buf_t *buf = NULL;
		printf("STDOUT:\n");
		fflush(stdout);
		ssize_t r = 0;
		buf = kexec_bufout(exec);
		do {
			void *d = NULL;
			ssize_t really_readed = 0;
			r = faux_buf_dread_lock_easy(buf, &d);
			if (r > 0) {
				really_readed = write(STDOUT_FILENO, d, r);
			}
			faux_buf_dread_unlock_easy(buf, really_readed);
		} while (r > 0);
	}

	kexec_free(exec);

	return BOOL_TRUE;
}
