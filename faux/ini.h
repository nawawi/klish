/** @file ini.h
 * @brief Public interface to work with ini files and strings.
 */

#ifndef _faux_ini_h
#define _faux_ini_h

#include "faux/types.h"
#include "faux/list.h"

typedef struct faux_pair_s faux_pair_t;
typedef struct faux_ini_s faux_ini_t;
typedef faux_list_node_t faux_ini_node_t;

C_DECL_BEGIN

// Pair
int faux_pair_compare(const void *first, const void *second);
faux_pair_t *faux_pair_new(const char *name, const char *value);
void faux_pair_free(void *pair);

const char *faux_pair_name(const faux_pair_t *pair);
void faux_pair_set_name(faux_pair_t *pair, const char *name);
const char *faux_pair_value(const faux_pair_t *pair);
void faux_pair_set_value(faux_pair_t *pair, const char *value);

// Ini
faux_ini_t *faux_ini_new(void);
void faux_ini_free(faux_ini_t *ini);

faux_ini_node_t *faux_ini_head(const faux_ini_t *ini);
faux_ini_node_t *faux_ini_tail(const faux_ini_t *ini);
faux_ini_node_t *faux_ini_next(const faux_ini_node_t *node);
faux_ini_node_t *faux_ini_prev(const faux_ini_node_t *node);
faux_pair_t *faux_ini_data(const faux_ini_node_t *node);

faux_pair_t *faux_ini_add(faux_ini_t *ini, const char *name, const char *value);
int faux_ini_parse_str(faux_ini_t *ini, const char *str);
int faux_ini_parse_file(faux_ini_t *ini, const char *fn);

faux_pair_t *faux_ini_find_pair(const faux_ini_t *ini, const char *name);
const char *faux_ini_find(const faux_ini_t *ini, const char *name);

C_DECL_END

#endif				/* _faux_ini_h */
