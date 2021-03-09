/** @file kptype.h
 *
 * @brief Klish scheme's "ptype" entry
 */

#ifndef _klish_kptype_h
#define _klish_kptype_h

#include <faux/error.h>
#include <klish/kaction.h>

typedef struct kptype_s kptype_t;


C_DECL_BEGIN

kptype_t *kptype_new(const char *name);
void kptype_free(kptype_t *ptype);

const char *kptype_name(const kptype_t *ptype);
const char *kptype_help(const kptype_t *ptype);
bool_t kptype_set_help(kptype_t *ptype, const char *help);

C_DECL_END

#endif // _klish_kptype_h
