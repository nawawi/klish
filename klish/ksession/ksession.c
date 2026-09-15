/** @file ksession.c
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <klish/khelper.h>
#include <klish/kscheme.h>
#include <klish/kpath.h>
#include <klish/ksession.h>


struct ksession_s {
	kscheme_t *scheme;
	kpath_t *path;
	bool_t done; // Indicates that session is over and must be closed
	size_t term_width;
	size_t term_height;
	pid_t spid; // Server process to serve current session
	pid_t pid; // Client's PID
	uid_t uid; // Client's UID
	char *user; // Client's user name (get by uid)
	bool_t isatty_std_in;
	bool_t isatty_std_out;
	bool_t isatty_std_err;
};


// Scheme
KGET(session, kscheme_t *, scheme);

// Path
KGET(session, kpath_t *, path);

// Done
KGET_BOOL(session, done);
KSET_BOOL(session, done);

// Width of pseudo terminal
KGET(session, size_t, term_width);
KSET(session, size_t, term_width);

// Height of pseudo terminal
KGET(session, size_t, term_height);
KSET(session, size_t, term_height);

// PID of server's service process
KGET(session, pid_t, spid);
KSET(session, pid_t, spid);

// PID of client (Unix socket peer)
KGET(session, pid_t, pid);
KSET(session, pid_t, pid);

// UID of client (Unix socket peer)
KGET(session, uid_t, uid);
KSET(session, uid_t, uid);

// Client user name (Unix socket peer)
KSET_STR(session, user);
KGET_STR(session, user);

// isatty
KGET_BOOL(session, isatty_std_in);
KSET_BOOL(session, isatty_std_in);
KGET_BOOL(session, isatty_std_out);
KSET_BOOL(session, isatty_std_out);
KGET_BOOL(session, isatty_std_err);
KSET_BOOL(session, isatty_std_err);


ksession_t *ksession_new(kscheme_t *scheme, const char *starting_entry)
{
	ksession_t *session = NULL;
	kentry_t *entry = NULL;
	klevel_t *level = NULL;

	assert(scheme);
	if (!scheme)
		return NULL;

	// Before real session allocation we will try to find starting entry.
	// Don't create session if we can't get starting entry.
	entry = kscheme_starting_entry(scheme, starting_entry);
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
	session->term_width = 0;
	session->term_height = 0;
	// Peer data
	session->pid = -1;
	session->uid = -1;
	session->user = NULL;
	session->isatty_std_in = BOOL_FALSE;
	session->isatty_std_out = BOOL_FALSE;
	session->isatty_std_err = BOOL_FALSE;
	session->spid = getpid(); // For forked processes

	return session;
}


void ksession_free(ksession_t *session)
{
	if (!session)
		return;

	kpath_free(session->path);
	faux_str_free(session->user);

	free(session);
}
