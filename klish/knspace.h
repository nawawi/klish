/** @file nspace.h
 *
 * @brief Klish scheme's "nspace" entry
 */

#ifndef _klish_knspace_h
#define _klish_knspace_h

#include <faux/faux.h>
#include <faux/list.h>
#include <klish/kview.h>


typedef struct knspace_s knspace_t;

// kview_t and knspace_t have a loop of references
typedef struct kview_s kview_t;


C_DECL_BEGIN

knspace_t *knspace_new(const char *view_ref);
void knspace_free(knspace_t *nspace);

// View_ref
const char *knspace_view_ref(const knspace_t *nspace);
// View
kview_t *knspace_view(const knspace_t *nspace);
bool_t knspace_set_view(knspace_t *nspace, kview_t *view);
// View
const char *knspace_prefix(const knspace_t *nspace);
bool_t knspace_set_prefix(knspace_t *nspace, const char *prefix);

C_DECL_END

#endif // _klish_knspace_h
