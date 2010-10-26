/*
 * clish_script_callback.c
 *
 *
 * Callback hook to action a shell script.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "private.h"

/*--------------------------------------------------------- */
bool_t clish_script_callback(const clish_shell_t * this,
	const clish_command_t * cmd, const char *script)
{
	FILE *wpipe;
	const char * shebang = NULL;

	assert(this);
	assert(cmd);
	if (!script) /* Nothing to do */
		return BOOL_TRUE;

	shebang = clish_command__get_shebang(cmd);
#ifdef DEBUG
	if (shebang)
		fprintf(stderr, "SHEBANG: #!%s\n", shebang);
	fprintf(stderr, "SYSTEM: %s\n", script);
#endif /* DEBUG */
	if (!shebang)
		return (0 == system(script)) ? BOOL_TRUE : BOOL_FALSE;
	/* The shebang is specified */
	wpipe = popen(shebang, "w");
	if (!wpipe)
		return BOOL_FALSE;
	fwrite(script, strlen(script) + 1, 1, wpipe);
	return (0 == pclose(wpipe)) ? BOOL_TRUE : BOOL_FALSE;
}

/*--------------------------------------------------------- */
bool_t clish_dryrun_callback(const clish_shell_t * this,
	const clish_command_t * cmd, const char *script)
{
#ifdef DEBUG
	fprintf(stderr, "DRY-RUN: %s\n", script);
#endif /* DEBUG */

	return BOOL_TRUE;
}

/*--------------------------------------------------------- */
