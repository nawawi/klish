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

#include "tcl.h"

#include "clish/private.h"
//---------------------------------------------------------
static clish_shell_hooks_t my_hooks = 
{
    tclish_init_callback,
    clish_access_callback,
    NULL, /* don't worry about cmd_line callback */
    tclish_script_callback,
    tclish_fini_callback,
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
    int              status = 0;
	tclish_cookie_t *cookie; 
    
    clish_startup(argc,argv);
    
    if(argc > 1)
    {
        int i = 1;
        while((0 == status) && argc--)
        {
            cookie = tclish_cookie_new(argv[0]);
            /* run the commands in the file */
            status = clish_shell_spawn_from_file(&my_hooks,cookie,argv[i++]);
        }
    }
    else
    {
        pthread_t pthread;
        void     *dummy;
        
        cookie = tclish_cookie_new(argv[0]);
        /* spawn the shell */
        status = clish_shell_spawn(&pthread,NULL,&my_hooks,cookie);

        if(-1 != status)
        {
            pthread_join(pthread,&dummy);
        }
    }
    if(-1 == status)
    {
        /* something went wrong */
        free(cookie);
    }
    
    (void)Tcl_FinalizeThread();
    clish_shutdown();

 	return status;
}
//---------------------------------------------------------
#else /* not HAVE_LIBTCL */
#include <stdio.h>
int main(void)
{
    printf("TCL not compiled in...\n");
}
#endif /* not HAVE_LIBTCL */
