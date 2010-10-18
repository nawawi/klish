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
int main(int argc, const char **argv)
{
	bool_t running;
	int result = -1;
	clish_shell_t * shell;

	shell = clish_shell_new(&my_hooks, NULL, stdin, stdout);
	if (!shell) {
		fprintf(stderr, "Cannot run clish.\n");
		return -1;
	}
	/* Load the XML files */
	clish_shell_load_files(shell);
	/* Execute startup */
	running = clish_shell_startup(shell);
	if (!running) {
		fprintf(stderr, "Cannot startup clish.\n");
		clish_shell_delete(shell);
		return -1;
	}

	if(argc > 1) {
		int i = 1;
		while(argc--) {
			/* Run the commands from the file */
			result = clish_shell_from_file(shell, argv[i++]);
		}
	} else {
		/* The interactive shell */
		result = clish_shell_loop(shell);
	}

	/* Cleanup */
	clish_shell_delete(shell);

	return result ? 0 : -1;
}
//---------------------------------------------------------
