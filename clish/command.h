/*
 * command.h
 */
#ifndef _clish_command_h
#define _clish_command_h

typedef struct clish_command_s clish_command_t;

#include "lub/bintree.h"
#include "lub/argv.h"
#include "clish/types.h"
#include "clish/pargv.h"
#include "clish/view.h"
#include "clish/param.h"

typedef enum {
	CLISH_CONFIG_NONE,
	CLISH_CONFIG_SET,
	CLISH_CONFIG_UNSET,
	CLISH_CONFIG_DUMP
} clish_config_operation_t;

/*=====================================
 * COMMAND INTERFACE
 *===================================== */
/*-----------------
 * meta functions
 *----------------- */
clish_command_t *clish_command_new(const char *name, const char *help);
clish_command_t *clish_command_new_link(const char *name,
	const char *help, const clish_command_t * ref);
clish_command_t * clish_command_alias_to_link(clish_command_t * instance);

int clish_command_bt_compare(const void *clientnode, const void *clientkey);
void clish_command_bt_getkey(const void *clientnode, lub_bintree_key_t * key);
size_t clish_command_bt_offset(void);
clish_command_t *clish_command_choose_longest(clish_command_t * cmd1,
	clish_command_t * cmd2);
int
clish_command_diff(const clish_command_t * cmd1, const clish_command_t * cmd2);
/*-----------------
 * methods
 *----------------- */
void clish_command_delete(clish_command_t * instance);
void
clish_command_insert_param(clish_command_t * instance, clish_param_t * param);
int clish_command_help(const clish_command_t * instance, clish_help_t *help,
	const char *viewid, const char *line);
void clish_command_dump(const clish_command_t * instance);

/*-----------------
 * attributes 
 *----------------- */
const char *clish_command__get_name(const clish_command_t * instance);
const char *clish_command__get_suffix(const clish_command_t * instance);
const char *clish_command__get_text(const clish_command_t * instance);
const char *clish_command__get_detail(const clish_command_t * instance);
const char *clish_command__get_builtin(const clish_command_t * instance);
const char *clish_command__get_escape_chars(const clish_command_t * instance);
const clish_param_t *clish_command__get_args(const clish_command_t * instance);
char *clish_command__get_action(const clish_command_t * instance,
	const char *viewid, clish_pargv_t * pargv);
clish_view_t *clish_command__get_view(const clish_command_t * instance);
char *clish_command__get_viewid(const clish_command_t * instance,
	const char *viewid, clish_pargv_t * pargv);
const unsigned clish_command__get_param_count(const clish_command_t * instance);
const clish_param_t *clish_command__get_param(const clish_command_t * instance,
	unsigned index);
clish_paramv_t *clish_command__get_paramv(const clish_command_t * instance);
void clish_command__set_action(clish_command_t * instance, const char *action);
void
clish_command__set_builtin(clish_command_t * instance, const char *builtin);
void
clish_command__set_escape_chars(clish_command_t * instance,
	const char *escape_chars);
void clish_command__set_args(clish_command_t * instance, clish_param_t * args);
void clish_command__set_detail(clish_command_t * instance, const char *detail);
void clish_command__set_view(clish_command_t * instance, clish_view_t * view);
void clish_command__force_view(clish_command_t * instance, clish_view_t * view);
void clish_command__set_viewid(clish_command_t * instance, const char *viewid);
void clish_command__force_viewid(clish_command_t * instance, const char *viewid);
void clish_command__set_pview(clish_command_t * instance, clish_view_t * view);
clish_view_t *clish_command__get_pview(const clish_command_t * instance);
unsigned clish_command__get_depth(const clish_command_t * instance);
void
clish_command__set_cfg_op(clish_command_t * instance,
	clish_config_operation_t operation);
clish_config_operation_t
clish_command__get_cfg_op(const clish_command_t * instance);
void clish_command__set_priority(clish_command_t * instance, unsigned short priority);
unsigned short clish_command__get_priority(const clish_command_t * instance);
void
clish_command__set_pattern(clish_command_t * instance, const char *pattern);
char *clish_command__get_pattern(const clish_command_t * instance,
	const char * viewid, clish_pargv_t * pargv);
void clish_command__set_file(clish_command_t * instance, const char *file);
char *clish_command__get_file(const clish_command_t * instance,
	const char * viewid, clish_pargv_t * pargv);
void clish_command__set_splitter(clish_command_t * instance, bool_t splitter);
bool_t clish_command__get_splitter(const clish_command_t * instance);
const char * clish_command__is_seq(const clish_command_t * instance);
void clish_command__set_seq(clish_command_t * instance, const char * seq_num);
unsigned short clish_command__get_seq(const clish_command_t * instance,
	const char *viewid, clish_pargv_t * pargv);
clish_view_restore_t clish_command__get_restore(const clish_command_t * instance);
bool_t clish_command__get_unique(const clish_command_t * instance);
void clish_command__set_unique(clish_command_t * instance, bool_t unique);
const clish_command_t * clish_command__get_orig(const clish_command_t * instance);
const clish_command_t * clish_command__get_cmd(const clish_command_t * instance);
void clish_command__set_cfg_depth(clish_command_t * instance, const char * cfg_depth);
unsigned clish_command__get_cfg_depth(const clish_command_t * instance,
	const char *viewid, clish_pargv_t * pargv);
bool_t clish_command__get_lock(const clish_command_t * instance);
void clish_command__set_lock(clish_command_t * instance, bool_t lock);
const char * clish_command__get_shebang(const clish_command_t * instance);
void clish_command__set_shebang(clish_command_t * instance, const char * shebang);

void clish_command__set_alias(clish_command_t * instance, const char * alias);
const char * clish_command__get_alias(const clish_command_t * instance);
void clish_command__set_alias_view(clish_command_t * instance,
	clish_view_t * alias_view);
clish_view_t * clish_command__get_alias_view(const clish_command_t * instance);
void clish_command__set_dynamic(clish_command_t * instance, bool_t dynamic);
bool_t clish_command__get_dynamic(const clish_command_t * instance);
bool_t clish_command__get_interrupt(const clish_command_t * instance);
void clish_command__set_interrupt(clish_command_t * instance, bool_t interrupt);

#endif				/* _clish_command_h */
