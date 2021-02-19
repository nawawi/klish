/** @file kptype.h
 *
 * @brief Klish scheme's "ptype" entry
 */

#ifndef _klish_kptype_h
#define _klish_kptype_h

typedef struct kptype_s kptype_t;

typedef struct iptype_s {
	char *name;
	char *help;
} iptype_t;


C_DECL_BEGIN

kptype_t *kptype_new(iptype_t info);
kptype_t *kptype_new_static(iptype_t info);
void kptype_free(kptype_t *ptype);

const char *kptype_name(const kptype_t *ptype);
const char *kptype_help(const kptype_t *ptype);

C_DECL_END

#endif // _klish_kptype_h
