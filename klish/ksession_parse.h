/** @file ksession_parse.h
 *
 * @brief Klish session parse
 */

#ifndef _klish_ksession_parse_h
#define _klish_ksession_parse_h

#include <klish/kpargv.h>
#include <klish/kexec.h>
#include <klish/ksession.h>


C_DECL_BEGIN

kpargv_t *ksession_parse_line(ksession_t *session, const faux_argv_t *argv,
	kpargv_purpose_e purpose, bool_t is_filter);
faux_list_t *ksession_split_pipes(const char *raw_line, faux_error_t *error);
kpargv_t *ksession_parse_for_completion(ksession_t *session,
	const char *raw_line);
kexec_t *ksession_parse_for_exec(ksession_t *session, const char *raw_line,
	faux_error_t *error);
kexec_t *ksession_parse_for_local_exec(ksession_t *session, const kentry_t *entry,
	const kpargv_t *parent_pargv, const kcontext_t *parent_context);
bool_t ksession_exec_locally(ksession_t *session, const kentry_t *entry,
	kpargv_t *parent_pargv, const kcontext_t *parent_context,
	int *retcode, char **out);

C_DECL_END

#endif // _klish_ksession_parse_h
