/*
 * standalone.h
 */
#include "lub/c_decl.h"
#include "clish/shell.h"

_BEGIN_C_DECL struct Tcl_Interp;

typedef struct tclish_cookie_s tclish_cookie_t;
struct tclish_cookie_s {
	struct Tcl_Interp *interp;
};

/* utility functions */
extern void clish_startup(int argc, const char **argv);
extern void clish_shutdown(void);
extern void tclish_show_result(struct Tcl_Interp *interp);

/* storage */
extern struct termios clish_default_tty_termios;

/* clish callback functions */
extern clish_shell_access_fn_t clish_access_callback;
extern clish_shell_script_fn_t clish_script_callback;
extern clish_shell_config_fn_t clish_config_callback;

/* tclish callback functions */
extern clish_shell_init_fn_t tclish_init_callback;
extern clish_shell_access_fn_t tclish_access_callback;
extern clish_shell_script_fn_t tclish_script_callback;
extern clish_shell_fini_fn_t tclish_fini_callback;

_END_C_DECL
