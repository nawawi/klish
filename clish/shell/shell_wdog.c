/*
 * shell_startup.c
 */
#include "private.h"
#include <assert.h>

#include "tinyrl/tinyrl.h"
#include "lub/string.h"

/*----------------------------------------------------------------------- */
int clish_shell_timeout_fn(tinyrl_t *this)
{
	tinyrl_crlf(this);
	fprintf(stderr, "Warning: Activity timeout. The session will be closed.\n");
	/* Return -1 to close session on timeout */
	return -1;
}

/*----------------------------------------------------------- */
int clish_shell_wdog(clish_shell_t *this)
{
	clish_context_t context;

	assert(this->wdog);

	context.shell = this;
	context.cmd = this->wdog;
	context.pargv = NULL;

	/* Call watchdog script */
	return clish_shell_execute(&context, NULL);
}

/*----------------------------------------------------------- */
void clish_shell__set_wdog_timeout(clish_shell_t *this, unsigned int timeout)
{
	assert(this);
	this->wdog_timeout = timeout;
}

/*----------------------------------------------------------- */
unsigned int clish_shell__get_wdog_timeout(const clish_shell_t *this)
{
	assert(this);
	return this->wdog_timeout;
}
