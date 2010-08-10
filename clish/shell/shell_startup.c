/*
 * shell_startup.c
 */
#include "private.h"
#include <assert.h>

/*----------------------------------------------------------- */
bool_t clish_shell_startup(clish_shell_t * this)
{
	const char *banner;
	clish_pargv_t *dummy = NULL;

	assert(this->startup);

	banner = clish_command__get_detail(this->startup);

	if (NULL != banner) {
		tinyrl_printf(this->tinyrl, "%s\n", banner);
	}
	return clish_shell_execute(this, this->startup, &dummy);
}

/*----------------------------------------------------------- */
