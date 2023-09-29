/** @file ksession.h
 *
 * @brief Klish session
 */

#ifndef _klish_ksession_h
#define _klish_ksession_h

#include <klish/kscheme.h>
#include <klish/kpath.h>

#define KSESSION_STARTING_ENTRY "main"

typedef struct ksession_s ksession_t;


C_DECL_BEGIN

ksession_t *ksession_new(kscheme_t *scheme, const char *start_entry);
void ksession_free(ksession_t *session);

kscheme_t *ksession_scheme(const ksession_t *session);
kpath_t *ksession_path(const ksession_t *session);

// Done
bool_t ksession_done(const ksession_t *session);
bool_t ksession_set_done(ksession_t *session, bool_t done);

// Width of pseudo terminal
size_t ksession_term_width(const ksession_t *session);
bool_t ksession_set_term_width(ksession_t *session, size_t term_width);

// Height of pseudo terminal
size_t ksession_term_height(const ksession_t *session);
bool_t ksession_set_term_height(ksession_t *session, size_t term_height);

// PID of client (Unix socket peer)
pid_t ksession_pid(const ksession_t *session);
bool_t ksession_set_pid(ksession_t *session, pid_t pid);

// UID of client (Unix socket peer)
uid_t ksession_uid(const ksession_t *session);
bool_t ksession_set_uid(ksession_t *session, uid_t uid);

// Client user name (Unix socket peer)
const char *ksession_user(const ksession_t *session);
bool_t ksession_set_user(ksession_t *session, const char *user);

// Client isatty
bool_t ksession_isatty_stdin(const ksession_t *session);
bool_t ksession_set_isatty_stdin(ksession_t *session, bool_t isatty_stdin);
bool_t ksession_isatty_stdout(const ksession_t *session);
bool_t ksession_set_isatty_stdout(ksession_t *session, bool_t isatty_stdout);
bool_t ksession_isatty_stderr(const ksession_t *session);
bool_t ksession_set_isatty_stderr(ksession_t *session, bool_t isatty_stderr);

C_DECL_END

#endif // _klish_ksession_h
