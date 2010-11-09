//-------------------------------------
// tclish.cpp
//
// A simple client for libclish using
// a TCL interpreter.
//
// (T)CL
// (C)ommand
// (L)ine
// (I)nterface
// (Sh)ell
//
// Pronounced: Ticklish
//-------------------------------------
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_LIBTCL
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <tcl.h>

#include "clish/shell.h"
#include "clish/internal.h"
#include "tcl_private.h"

//---------------------------------------------------------
static clish_shell_hooks_t my_hooks = 
{
    tclish_init_callback,
    clish_access_callback,
    NULL, /* don't worry about cmd_line callback */
    tclish_script_callback,
    tclish_fini_callback,
    NULL, /* don't worry about config callback */
    NULL
};
//---------------------------------------------------------
static void
tclish_cookie_init(tclish_cookie_t *cookie,
                   const char      *argv0)
{
    /* make sure that the platform specific details are set up */
    Tcl_FindExecutable(argv0);

    cookie->interp     = NULL;
}
//---------------------------------------------------------
static tclish_cookie_t *
tclish_cookie_new(const char *argv0)
{
    tclish_cookie_t *cookie = (tclish_cookie_t *)malloc(sizeof(tclish_cookie_t));
    
    if(NULL != cookie)
    {
        tclish_cookie_init(cookie,argv0);
    }
    return cookie;
}
//---------------------------------------------------------
int 
main(int argc, const char **argv)
{
	tclish_cookie_t *cookie;
	bool_t running;
	int result = -1;
	clish_shell_t * shell;
	bool_t lockless = BOOL_FALSE;
	bool_t stop_on_error = BOOL_FALSE;
	const char *xml_path = getenv("CLISH_PATH");
	const char *view = getenv("CLISH_VIEW");
	const char *viewid = getenv("CLISH_VIEWID");

	cookie = tclish_cookie_new(argv[0]);
	/* Create shell instance */
	shell = clish_shell_new(&my_hooks, cookie, NULL, stdout, stop_on_error);
	if (!shell) {
		fprintf(stderr, "Cannot run clish.\n");
		return -1;
	}
	/* Load the XML files */
	clish_shell_load_scheme(shell, xml_path);
	/* Set communication to the konfd */
//	clish_shell__set_socket(shell, socket_path);
	/* Set lockless mode */
	if (lockless)
		clish_shell__set_lockfile(shell, NULL);
	/* Set startup view */
	if (view)
		clish_shell__set_startup_view(shell, view);
	/* Set startup viewid */
	if (viewid)
		clish_shell__set_startup_viewid(shell, viewid);
	/* Execute startup */
	running = clish_shell_startup(shell);
	if (!running) {
		fprintf(stderr, "Cannot startup clish.\n");
		clish_shell_delete(shell);
		return -1;
	}

	if(argc > 1) {
		int i;
		/* Run the commands from the files */
		for (i = argc - 1; i >= 1; i--)
			clish_shell_push_file(shell, argv[i], stop_on_error);
	} else {
		/* The interactive shell */
		clish_shell_push_fd(shell, fdopen(dup(fileno(stdin)), "r"), stop_on_error);
	}
	result = clish_shell_loop(shell);

	/* Cleanup */
	clish_shell_delete(shell);
	free(cookie);
	(void)Tcl_FinalizeThread();

	return result ? 0 : -1;
}
//---------------------------------------------------------
#else /* not HAVE_LIBTCL */
#include <stdio.h>
int main(void)
{
    printf("TCL not compiled in...\n");
}
#endif /* not HAVE_LIBTCL */
