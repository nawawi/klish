/*
 * shell.h
 */
 /**
\ingroup clish
\defgroup clish_shell shell
@{

\brief This class represents the top level container for a CLI session.

*/
#ifndef _clish_shell_h
#define _clish_shell_h

#include <stdio.h>
#include <pthread.h>

#include "lub/c_decl.h"
#include "lub/types.h"
#include "lub/argv.h"

#include "tinyrl/tinyrl.h"

#include "view.h"
#include "cliconf/net.h"

_BEGIN_C_DECL typedef struct clish_shell_s clish_shell_t;

/*=====================================
 * SHELL INTERFACE
 *===================================== */
 /**
  * A hook function used during the spawning of a new shell.
  * 
  * This will be invoked from the context of the spawned shell's thread
  * and will be invoked just after the shell instance is created.
  *
  * This enables the client-specific initialisation of the spawned shell's
  * thread
  * e.g. to map the I/O streams, authenticate a user.
  *
  * N.B. It is possible for a client to have this invoked multiple times 
  * if the user is spawning new shells using a commmand which uses the 
  * "clish_spawn" builtin function. Hence the client should remember the 
  * shell which first calls this function, and only assign resource (e.g. 
  * setting up a script interpreter) for that call.
  *
  * \return
  * - BOOL_TRUE if everything is OK
  * - BOOL_FALSE if the shell should be immediately shut down.
  *
  */
typedef bool_t clish_shell_init_fn_t(
	/** 
         * The shell instance which invoked this call
         */
					    const clish_shell_t * shell);

 /**
  * A hook function used during the shutting down of a spawned shell
  * 
  * This will be invoked from the context of the spawned shell's thread
  * and will be invoked just before the shell is destroyed.
  *
  * This enables the client-specific finalisation to occur.
  * e.g. releasing any resource held by the cookie,
  * shutting down telnet connections
  *
  * NB. This function may be called multiple times if a user is spawning
  * new commands (via the "clish_spawn" builtin command), hence should use
  * the reference to the root shell (remembered by the first call to clish_shell_init_fn_t callback)
  * to signal when the cleanup should occur.
  */
typedef void clish_shell_fini_fn_t(
	/** 
         * The shell instance which invoked this call
         */
					  const clish_shell_t * shell);

 /**
  * A hook function used to indicate a command line has been executed and the 
  * shell is about to prompt for the next command.
  * 
  * This will be invoked from the context of the spawned shell's thread
  * and will be called once an ACTION has been performed.
  * 
  * A client may use this as a periodic indicator of CLI activity, 
  * e.g. to manage session timeouts. In addition any required logging of 
  * commands may be performed.
  */
typedef void clish_shell_cmd_line_fn_t(
	/** 
         * The shell instance which invoked this call
         */
					      const clish_shell_t * instance,
	/** 
         * The text of the command line entered
         */
					      const char *cmd_line);

/**
  * A hook function used to invoke the script associated with a command
  * 
  * This will be invoked from the context of the spawned shell's thread
  * and will be invoked with the ACTION script which is to be performed.
  * 
  * The clish component will only pass down a call when a command has been
  * correctly input.
  *
  * The client may choose to implement invocation of the script in a number of
  * ways, which may include forking a sub-process or thread. It is important 
  * that the call doesn't return until the script has been fully evaluated.
  * 
  * \return 
  * - BOOL_TRUE  - if the script is executed without issue
  * - BOOL_FALSE - if the script had an issue with execution.
  *
  * \post
  * - If the script executes successfully then any "view" tag associated with the
  *   command will be honored. i.e. the CLI will switch to the new view
  */
typedef bool_t clish_shell_script_fn_t(
	/** 
         * The shell instance which invoked this call
         */
					      const clish_shell_t * instance,
	/** 
         * The script to be evaluated
         */
					      const char *script);

/**
  * A hook function used to control config file write
  * 
  */
typedef bool_t clish_shell_config_fn_t(
	/** 
         * The shell instance which invoked this call
         */
					      const clish_shell_t * instance,
	/** 
         * The command
         */
					      const clish_command_t * cmd,
	/** 
         * Argument vector
         */
					      clish_pargv_t * pargv);

 /**
  * A hook function used to control access for the current user.
  * 
  * This will be invoked from the context of the spawned shell's thread
  * and will be called during the parsing of the XML files.
  * 
  * The clish component will only insert a command into a view if the access 
  * call is sucessfull.
  *
  * The client may choose to implement invocation of the script in a number of
  * ways, which may include forking a sub-process or thread. It is important 
  * that the call doesn't return until the script has been fully evaluated.
  *
  * \return
  * - BOOL_TRUE  - if the user of the current CLISH session is permitted access
  * - BOOL_FALSE - if the user of the current CLISH session is not permitted access
  *
  * \post
  * - If access is granted then the associated command will be inserted into the
  *   appropriate view.
  */
typedef bool_t clish_shell_access_fn_t(
	/** 
         * The shell instance which invoked this call
         */
					      const clish_shell_t * instance,
	/**
         * A textual string which describes a limitation for a command. This
         * string typically may be the name of a user group, of which the
         * current user must be a member to grant access to a command.  
         */
					      const char *access);
 /**
  * A hook function used as a built in command callback
  * 
  * This will be invoked from the context of the spawned shell's thread
  * and will be called during the execution of a builting command.
  * 
  * A client may register any number of these callbacks in its 
  * clish_shell_builtin_cmds_t structure.
  *
  * \return
  * - BOOL_TRUE  - if the command completes correctly
  * - BOOL_FALSE - if the command fails.
  *
  */
typedef bool_t clish_shell_builtin_fn_t(
	/** 
         * The shell instance which invoked this call
         */
					       const clish_shell_t * instance,
	/** 
         * A vector of textual command line arguments.
         */
					       const lub_argv_t * argv);

/** 
 * A client of libclish may provide some builtin commands which will be
 * interpreted by the framework, instead of the client's script engine.
 */
typedef struct {
	const char *name;		/**< The textual name to be used in 
                                         *    the 'builtin' XML attribute"
                                         */
	clish_shell_builtin_fn_t *callback;
					/**< The function to be invoked */
} clish_shell_builtin_t;

/** 
 * A client of libclish will provide hooks for the control of the CLI within 
 * a particular system.
 * They will populate an instance of this structure and pass it into the 
 */
typedef struct {
	clish_shell_init_fn_t *init_fn;	       /**< Initialisation call       */
	clish_shell_access_fn_t *access_fn;    /**< Access control call       */
	clish_shell_cmd_line_fn_t *cmd_line_fn;/**< Command line logging call */
	clish_shell_script_fn_t *script_fn;    /**< script evaluation call    */
	clish_shell_fini_fn_t *fini_fn;	       /**< Finalisation call         */
	clish_shell_config_fn_t *config_fn;    /**< Config call               */
	const clish_shell_builtin_t *cmd_list;/**< NULL terminated list      */
} clish_shell_hooks_t;
/*-----------------
 * meta functions
 *----------------- */
int clish_shell_spawn_and_wait(const clish_shell_hooks_t * hooks, void *cookie);
 /**
  * This operation causes a separate (POSIX) thread of execution to 
  * be spawned. This thread becomes responsible for the CLI session.
  * 
  * This will be invoked from the context of the spawned shell's thread
  * and will be called during the execution of a builting command.
  * 
  * A client may register any number of these callbacks in its 
  * clish_shell_builtin_cmds_t structure.
  *
  * \return
  * - BOOL_TRUE  - if the thread was successfully spawned
  * - BOOL_FALSE - if the thread failed to be spawned
  *
  */
bool_t clish_shell_spawn(
	/** 
         * A POSIX thread reference to fill out. This can be used
         * to later control the spawned thread if necessary.
         */
				pthread_t * pthread,
	/** 
         * A POSIX thread attribute reference which will be used
         * to define the thread which will be lanched. A value of
         * NULL will use the system default.
         */
				const pthread_attr_t * attr,
	/** 
          * A reference to the clients hooks. These are used to 
          * communicate back the client when client-specific actions
          * are required.
          */
				const clish_shell_hooks_t * hooks,
	/** 
         * A client specific reference which can be obtained during
         * a callback by invoking clish_shell__get_client_cookie()
         */
				void *cookie);
bool_t
clish_shell_spawn_from_file(const clish_shell_hooks_t * hooks,
			    void *cookie, const char *filename);

clish_shell_t *clish_shell_new(const clish_shell_hooks_t * hooks,
			       void *cookie, FILE * istream);
/*-----------------
 * methods
 *----------------- */
/*
 * Called to invoke the startup command for this shell
 */
bool_t clish_shell_startup(clish_shell_t * instance);
void clish_shell_delete(clish_shell_t * instance);
clish_view_t *clish_shell_find_create_view(clish_shell_t * instance,
					   const char *name,
					   const char *prompt);
clish_ptype_t *clish_shell_find_create_ptype(clish_shell_t * instance,
					     const char *name,
					     const char *text,
					     const char *pattern,
					     clish_ptype_method_e method,
					     clish_ptype_preprocess_e
					     preprocess);
int clish_shell_xml_read(clish_shell_t * instance, const char *filename);
void clish_shell_help(clish_shell_t * instance, const char *line);
bool_t
clish_shell_execute(clish_shell_t * instance,
		    const clish_command_t * cmd, clish_pargv_t ** pargv);
bool_t
clish_shell_readline(clish_shell_t * shell,
		     const char *prompt,
		     const clish_command_t ** cmd, clish_pargv_t ** pargv);
void clish_shell_set_context(clish_shell_t * instance, const char *viewname);
void clish_shell_dump(clish_shell_t * instance);
void clish_shell_close(clish_shell_t * instance);
/*-----------------
 * attributes 
 *----------------- */
const clish_view_t *clish_shell__get_view(const clish_shell_t * instance);
const char *clish_shell__get_viewid(const clish_shell_t * instance);
const char *clish_shell__get_overview(const clish_shell_t * instance);
tinyrl_t *clish_shell__get_tinyrl(const clish_shell_t * instance);
void *clish_shell__get_client_cookie(const clish_shell_t * instance);
void
clish_shell__set_pwd(clish_shell_t * instance, unsigned index,
		     const char *element);
const char *clish_shell__get_pwd(const clish_shell_t * instance,
				 unsigned index);
char *clish_shell__get_line(const clish_command_t * cmd, clish_pargv_t * pargv);
conf_client_t *clish_shell__get_client(const clish_shell_t * instance);

_END_C_DECL
#endif				/* _clish_shell_h */
/** @} clish_shell */
