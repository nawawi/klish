/*
 * tclish_show_result.c
 *
 * Display the last result for the specified interpreter.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif				/* HAVE_CONFIG_H */

#ifdef HAVE_LIBTCL
#include <assert.h>

#include <tcl.h>

#include "private.h"
/*--------------------------------------------------------- */
void tclish_show_result(Tcl_Interp * interp)
{
	Tcl_Obj *obj = Tcl_GetObjResult(interp);
	int length;
	if (NULL != obj) {
		char *string = Tcl_GetStringFromObj(obj, &length);
		if (NULL != string) {
			printf("%s", string);
		}
	}
}

/*--------------------------------------------------------- */
#endif				/* HAVE_LIBTCL */
