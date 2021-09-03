/** @file ientry.h
 *
 * @brief Klish scheme's "entry" entry
 */

#ifndef _klish_ientry_h
#define _klish_ientry_h

#include <faux/error.h>
#include <klish/iaction.h>
#include <klish/kentry.h>

typedef struct ientry_s ientry_t;

struct ientry_s {
	char *name;
	char *help;
	char *container;
	char *mode;
	char *purpose;
	char *min;
	char *max;
	char *ptype;
	char *ref;
	char *value;
	char *restore;
	char *order;
	char *filter;
	ientry_t * (*entrys)[]; // Nested entrys
	iaction_t * (*actions)[];
};

C_DECL_BEGIN

bool_t ientry_parse(const ientry_t *info, kentry_t *entry, faux_error_t *error);
bool_t ientry_parse_nested(const ientry_t *ientry, kentry_t *kentry,
	faux_error_t *error);
kentry_t *ientry_load(const ientry_t *ientry, faux_error_t *error);
char *ientry_deploy(const kentry_t *kentry, int level);

C_DECL_END

#endif // _klish_ientry_h
