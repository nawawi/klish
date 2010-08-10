/*
 * tclish_script_callback.c
 *
 *
 * Callback hook to action a TCL script.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif				/* HAVE_CONFIG_H */

#ifdef HAVE_LIBTCL

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <tcl.h>

#include "private.h"
/*--------------------------------------------------------- */
bool_t tclish_script_callback(const clish_shell_t * shell, const char *script)
{
	bool_t result = BOOL_TRUE;
	tclish_cookie_t *this = clish_shell__get_client_cookie(shell);

	if (Tcl_CommandComplete(script)) {
		if (TCL_OK != Tcl_EvalEx(this->interp,
					 script,
					 strlen(script), TCL_EVAL_GLOBAL)) {
			result = BOOL_FALSE;
		}
		tclish_show_result(this->interp);
	}
	return result;
}

/*--------------------------------------------------------- */
#endif				/* HAVE_LIBTCL */
