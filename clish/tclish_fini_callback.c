/*
 * tclish_fini_callback.c
 *
 *  Callback to restore the terminal settings
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif				/* HAVE_CONFIG_H */

#ifdef HAVE_LIBTCL
#include <stdlib.h>
#include <tcl.h>

#include "private.h"
/*--------------------------------------------------------- */
void tclish_fini_callback(const clish_shell_t * shell)
{
	tclish_cookie_t *this = clish_shell__get_client_cookie(shell);

	/* cleanup the TCL interpreter */
	(void)Tcl_Release(this->interp);
	(void)Tcl_DeleteInterp(this->interp);

	/* free up memory */
	free(this);
}

/*--------------------------------------------------------- */
#endif				/* HAVE_LIBTCL */
