//-------------------------------------
// clish.cpp
//
// A simple client for libclish
//-------------------------------------
#include "clish/private.h"

static 
clish_shell_hooks_t my_hooks = 
{
    NULL, /* don't worry about init callback */
    clish_access_callback,
    NULL, /* don't worry about cmd_line callback */
    clish_script_callback,
    NULL, /* don't worry about fini callback */
    clish_config_callback,
    NULL  /* don't register any builtin functions */
};
//---------------------------------------------------------
int 
main(int argc, const char **argv)
{
	int            result      = -1;
	
    clish_startup(argc,argv);
    
    if(argc > 1)
    {
        int i = 1;
        while(argc--)
        {
            /* run the commands in the file */
            result = clish_shell_spawn_from_file(&my_hooks,NULL,argv[i++]);
        }
    }
    else
    {
        /* spawn the shell */
        result = clish_shell_spawn_and_wait(&my_hooks,NULL);
    }

    clish_shutdown();

 	return result ? 0 : -1;
 }
//---------------------------------------------------------
