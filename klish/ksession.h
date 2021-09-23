/** @file ksession.h
 *
 * @brief Klish session
 */

#ifndef _klish_ksession_h
#define _klish_ksession_h

#include <klish/kscheme.h>
#include <klish/kpath.h>
#include <klish/kpargv.h>
#include <klish/kexec.h>

#define KSESSION_STARTING_ENTRY "main"

typedef struct ksession_s ksession_t;


C_DECL_BEGIN

ksession_t *ksession_new(const kscheme_t *scheme, const char *start_entry);
void ksession_free(ksession_t *session);

const kscheme_t *ksession_scheme(const ksession_t *session);
kpath_t *ksession_path(const ksession_t *session);

// Done
bool_t ksession_done(const ksession_t *session);
bool_t ksession_set_done(ksession_t *session, bool_t done);

kpargv_t *ksession_parse_line(ksession_t *session, const faux_argv_t *argv,
	kpargv_purpose_e purpose);
faux_list_t *ksession_split_pipes(const char *raw_line, faux_error_t *error);
kpargv_t *ksession_parse_for_completion(ksession_t *session,
	const char *raw_line);
kexec_t *ksession_parse_for_exec(ksession_t *session, const char *raw_line,
	faux_error_t *error);
kexec_t *ksession_parse_for_local_exec(ksession_t *session,
	const kentry_t *entry, const kpargv_t *parent_pargv);

bool_t ksession_exec_locally(ksession_t *session, const kentry_t *entry,
	kpargv_t *parent_pargv, int *retcode, const char **out);

C_DECL_END

#endif // _klish_ksession_h
