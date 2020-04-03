/** @file ctype.h
 * @brief Public interface for faux ctype functions.
 */

#ifndef _faux_ctype_h
#define _faux_ctype_h

#include <ctype.h>

#include "faux/types.h"
#include "faux/c_decl.h"

C_DECL_BEGIN

// Classify functions
bool_t faux_ctype_isdigit(char c);
bool_t faux_ctype_isspace(char c);

// Convert functions
char faux_ctype_tolower(char c);
char faux_ctype_toupper(char c);

C_DECL_END

#endif /* _faux_ctype_h */
