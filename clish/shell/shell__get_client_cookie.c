/*
 * shell__get_client_cookie.c
 */
#include "private.h"

/*--------------------------------------------------------- */
void *clish_shell__get_client_cookie(const clish_shell_t * this)
{
	return this->client_cookie;
}

/*--------------------------------------------------------- */
