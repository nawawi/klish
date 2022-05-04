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

C_DECL_END

#endif // _klish_ksession_h
