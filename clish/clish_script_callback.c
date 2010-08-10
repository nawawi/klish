/*
 * clish_script_callback.c
 *
 *
 * Callback hook to action a shell script.
 */
#include <stdio.h>
#include <stdlib.h>

#include "private.h"
/*--------------------------------------------------------- */
bool_t clish_script_callback(const clish_shell_t * shell, const char *script)
{
#ifdef DEBUG
	printf("SYSTEM: %s\n", script);
#endif				/* DEBUG */

	return (0 == system(script)) ? BOOL_TRUE : BOOL_FALSE;
}

/*--------------------------------------------------------- */
