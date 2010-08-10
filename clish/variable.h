/*
 * variable.h
 */
 /**
\ingroup clish
\defgroup clish_variable variable
@{

\brief This utility is used to expand variables within a string.

*/
#ifndef _clish_variable_h
#define _clish_variable_h

#include "clish/shell.h"
#include "clish/command.h"
#include "clish/pargv.h"

/*=====================================
 * VARIABLE INTERFACE
 *===================================== */
/*-----------------
 * meta functions
 *----------------- */
char *clish_variable_expand(const char *string,
			    const char *viewid,
			    const clish_command_t * cmd, clish_pargv_t * pargv);
/*-----------------
 * methods
 *----------------- */
char *clish_variable__get_line(const clish_command_t * cmd, clish_pargv_t * pargv);

/*-----------------
 * attributes
 *----------------- */

#endif				/* _clish_variable_h */
/** @} clish_variable */
