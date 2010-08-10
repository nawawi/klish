/*
 * clish_shutdown.c
 */
#include "private.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif				/* DMALLOC */

/*--------------------------------------------------------- */
void clish_shutdown(void)
{
#ifdef DMALLOC
	dmalloc_shutdown();
#endif
}

/*--------------------------------------------------------- */
