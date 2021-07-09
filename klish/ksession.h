/** @file ksession.h
 *
 * @brief Klish session
 */

#ifndef _klish_ksession_h
#define _klish_ksession_h

#include <klish/kscheme.h>
#include <klish/kpath.h>
#include <klish/kpargv.h>

#define KSESSION_STARTING_ENTRY "main"

typedef struct ksession_s ksession_t;


C_DECL_BEGIN

ksession_t *ksession_new(const kscheme_t *scheme, const char *start_entry);
void ksession_free(ksession_t *session);

const kscheme_t *ksession_scheme(const ksession_t *session);
kpath_t *ksession_path(const ksession_t *session);

kpargv_t *ksession_parse_line(ksession_t *session, const char *line,
	kpargv_purpose_e purpose);
faux_list_t *ksession_split_pipes(const char *line);

C_DECL_END

#endif // _klish_ksession_h
