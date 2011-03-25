/*
 * private.h
 */

#ifndef _clish_private_h
#define _clish_private_h

#include "lub/c_decl.h"
#include "lub/argv.h"
/*#include "clish/internal.h"*/

struct help_argv_s {
	lub_argv_t *matches;
	lub_argv_t *help;
	lub_argv_t *detail;
};
typedef struct help_argv_s help_argv_t;

#endif /* _clish_private_h */
