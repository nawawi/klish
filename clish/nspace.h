/*
 * nspace.h
 * Namespace instances are assocated with a view to make view's commands available
 * within current view.
 */

#ifndef _clish_nspace_h
#define _clish_nspace_h

typedef struct clish_nspace_s clish_nspace_t;

typedef enum {
	CLISH_NSPACE_NONE,
	CLISH_NSPACE_HELP,
	CLISH_NSPACE_COMPLETION,
	CLISH_NSPACE_CHELP
} clish_nspace_visibility_e;

#include <regex.h>

#include "clish/macros.h"
#include "clish/view.h"

clish_nspace_t *clish_nspace_new(const char *view_name);

void clish_nspace_delete(void *instance);
const clish_command_t *clish_nspace_find_next_completion(clish_nspace_t *
	instance, const char *iter_cmd, const char *line,
	clish_nspace_visibility_e field);
clish_command_t *clish_nspace_find_command(clish_nspace_t * instance, const char *name);
void clish_nspace_dump(const clish_nspace_t * instance);
clish_command_t * clish_nspace_create_prefix_cmd(clish_nspace_t * instance,
	const char * name, const char * help);
void clish_nspace_clean_proxy(clish_nspace_t * instance);

_CLISH_SET(nspace, bool_t, help);
_CLISH_GET(nspace, bool_t, help);
_CLISH_SET(nspace, bool_t, completion);
_CLISH_GET(nspace, bool_t, completion);
_CLISH_SET(nspace, bool_t, context_help);
_CLISH_GET(nspace, bool_t, context_help);
_CLISH_SET(nspace, bool_t, inherit);
_CLISH_GET(nspace, bool_t, inherit);
_CLISH_SET(nspace, clish_view_t *, view);
_CLISH_GET(nspace, clish_view_t *, view);
_CLISH_SET_STR(nspace, view_name);
_CLISH_GET_STR(nspace, view_name);
_CLISH_SET_STR(nspace, access);
_CLISH_GET_STR(nspace, access);
_CLISH_SET_STR_ONCE(nspace, prefix);
_CLISH_GET_STR(nspace, prefix);
_CLISH_GET(nspace, const regex_t *, prefix_regex);

bool_t clish_nspace__get_visibility(const clish_nspace_t * instance,
	clish_nspace_visibility_e field);

#endif				/* _clish_nspace_h */
