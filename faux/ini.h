/** @file ini.h
 * @brief Public interface to work with ini files and strings.
 */

#ifndef _faux_ini_h
#define _faux_ini_h

#include "faux/faux.h"
#include "faux/list.h"

typedef struct faux_pair_s faux_pair_t;
typedef struct faux_ini_s faux_ini_t;
typedef faux_list_node_t faux_ini_node_t;

C_DECL_BEGIN

// Pair
const char *faux_pair_name(const faux_pair_t *pair);
const char *faux_pair_value(const faux_pair_t *pair);

// Ini
faux_ini_t *faux_ini_new(void);
void faux_ini_free(faux_ini_t *ini);

const faux_pair_t *faux_ini_set(faux_ini_t *ini, const char *name, const char *value);
void faux_ini_unset(faux_ini_t *ini, const char *name);

const faux_pair_t *faux_ini_find_pair(const faux_ini_t *ini, const char *name);
const char *faux_ini_find(const faux_ini_t *ini, const char *name);
faux_ini_node_t *faux_ini_init_iter(const faux_ini_t *ini);
const faux_pair_t *faux_ini_each(faux_ini_node_t **iter);

int faux_ini_parse_str(faux_ini_t *ini, const char *str);
int faux_ini_parse_file(faux_ini_t *ini, const char *fn);

C_DECL_END

#endif				/* _faux_ini_h */
