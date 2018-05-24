/*
 * command.h
 */

#ifndef _clish_command_h
#define _clish_command_h

typedef struct clish_command_s clish_command_t;

#include "lub/bintree.h"
#include "lub/argv.h"
#include "clish/types.h"
#include "clish/macros.h"
#include "clish/pargv.h"
#include "clish/view.h"
#include "clish/param.h"
#include "clish/action.h"
#include "clish/config.h"

clish_command_t *clish_command_new(const char *name, const char *help);
clish_command_t *clish_command_new_link(const char *name,
	const char *help, const clish_command_t * ref);
clish_command_t * clish_command_alias_to_link(clish_command_t *instance, clish_command_t *ref);

int clish_command_bt_compare(const void *clientnode, const void *clientkey);
void clish_command_bt_getkey(const void *clientnode, lub_bintree_key_t * key);
size_t clish_command_bt_offset(void);
clish_command_t *clish_command_choose_longest(clish_command_t * cmd1,
	clish_command_t * cmd2);
int
clish_command_diff(const clish_command_t * cmd1, const clish_command_t * cmd2);

void clish_command_delete(clish_command_t *instance);
void clish_command_insert_param(clish_command_t *instance,
	clish_param_t *param);
int clish_command_help(const clish_command_t *instance);
void clish_command_dump(const clish_command_t *instance);

_CLISH_GET_STR(command, name);
_CLISH_GET_STR(command, text);
_CLISH_SET_STR_ONCE(command, detail);
_CLISH_GET_STR(command, detail);
_CLISH_GET(command, clish_action_t *, action);
_CLISH_GET(command, clish_config_t *, config);
_CLISH_SET_STR_ONCE(command, regex_chars);
_CLISH_GET_STR(command, regex_chars);
_CLISH_SET_STR_ONCE(command, escape_chars);
_CLISH_GET_STR(command, escape_chars);
_CLISH_SET_STR_ONCE(command, viewname);
_CLISH_GET_STR(command, viewname);
_CLISH_SET_STR_ONCE(command, viewid);
_CLISH_GET_STR(command, viewid);
_CLISH_SET_ONCE(command, clish_param_t *, args);
_CLISH_GET(command, clish_param_t *, args);
_CLISH_GET(command, clish_paramv_t *, paramv);
_CLISH_SET(command, clish_view_t *, pview);
_CLISH_GET(command, clish_view_t *, pview);
_CLISH_SET_STR(command, access);
_CLISH_GET_STR(command, access);
_CLISH_SET_STR(command, alias);
_CLISH_GET_STR(command, alias);
_CLISH_SET_STR(command, alias_view);
_CLISH_GET_STR(command, alias_view);
_CLISH_SET(command, bool_t, internal);
_CLISH_GET(command, bool_t, internal);
_CLISH_SET(command, bool_t, dynamic);
_CLISH_GET(command, bool_t, dynamic);

const char *clish_command__get_suffix(const clish_command_t * instance);
unsigned int clish_command__get_param_count(const clish_command_t * instance);
const clish_param_t *clish_command__get_param(const clish_command_t * instance,
	unsigned index);
void clish_command__force_viewname(clish_command_t * instance, const char *viewname);
void clish_command__force_viewid(clish_command_t * instance, const char *viewid);
int clish_command__get_depth(const clish_command_t * instance);
clish_view_restore_e clish_command__get_restore(const clish_command_t * instance);
const clish_command_t * clish_command__get_orig(const clish_command_t * instance);
const clish_command_t * clish_command__get_cmd(const clish_command_t * instance);

#endif				/* _clish_command_h */
