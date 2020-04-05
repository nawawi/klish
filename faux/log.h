/** @file log.h
 * @brief Public interface for faux log functions.
 */

#ifndef _faux_log_h
#define _faux_log_h

#include "faux/types.h"

C_DECL_BEGIN

int faux_log_facility(const char *str, int *facility);

C_DECL_END

#endif
