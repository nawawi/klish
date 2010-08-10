/*
 * tclish_init_callback.c
 *
 * Callback to set the initialise the terminal
 * - set raw mode (block for a single character)
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif				/* HAVE_CONFIG_H */

#ifdef HAVE_LIBTCL
#include <assert.h>

#include <tcl.h>

#include "private.h"
/*--------------------------------------------------------- */
bool_t tclish_init_callback(const clish_shell_t * shell)
{
	int result;
	tclish_cookie_t *this = clish_shell__get_client_cookie(shell);

	/* initialise the TCL interpreter */
	this->interp = Tcl_CreateInterp();
	assert(NULL != this->interp);
	/* initialise the memory */
	Tcl_InitMemory(interp);
	result = Tcl_Init(this->interp);
	if (TCL_OK != result) {
		tclish_show_result(this->interp);
	}
	Tcl_Preserve(this->interp);

	return BOOL_TRUE;
}

/*--------------------------------------------------------- */
#endif				/* HAVE_LIBTCL */
