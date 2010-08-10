/*
 * private.h
 */
#include "clish/variable.h"

/*--------------------------------------------------------- */
typedef struct context_s context_t;
struct context_s {
	const char *viewid;
	const clish_command_t *cmd;
	clish_pargv_t *pargv;
};
/*--------------------------------------------------------- */
