/*
 * udata.h
 */
 /**
\ingroup clish
\defgroup clish_udata udata
@{

\brief This class represents the top level container for CLI user data.
*/
#ifndef _clish_udata_h
#define _clish_udata_h

#include "clish/macros.h"

typedef struct clish_udata_s clish_udata_t;

int clish_udata_compare(const void *first, const void *second);
clish_udata_t *clish_udata_new(const char *name, void *data);
void *clish_udata_free(clish_udata_t *instance);
void clish_udata_delete(void *data);

_CLISH_SET(udata, void *, data);
_CLISH_GET(udata, void *, data);
_CLISH_GET_STR(udata, name);

#endif				/* _clish_udata_h */
/** @} clish_udata */
