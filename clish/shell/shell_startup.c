/*
 * shell_startup.c
 */
#include "private.h"
#include <assert.h>

/*----------------------------------------------------------- */
bool_t clish_shell_startup(clish_shell_t * this)
{
	const char *banner;

	assert(this->startup);

	banner = clish_command__get_detail(this->startup);

	if (NULL != banner) {
		tinyrl_printf(this->tinyrl, "%s\n", banner);
	}
	return clish_shell_execute(this, this->startup, NULL);
}

/*----------------------------------------------------------- */
void clish_shell__set_startup_view(clish_shell_t * this, const char * viewname)
{
	clish_view_t *view;

	assert(this);
	assert(this->startup);
	/* Search for the view */
	view = clish_shell_find_create_view(this, viewname, NULL);
	clish_command__force_view(this->startup, view);
}

/*----------------------------------------------------------- */
void clish_shell__set_startup_viewid(clish_shell_t * this, const char * viewid)
{
	assert(this);
	assert(this->startup);
	clish_command__force_viewid(this->startup, viewid);
}

/*----------------------------------------------------------- */
