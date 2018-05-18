#ifndef _clish_var_h
#define _clish_var_h

#include "lub/types.h"
#include "lub/bintree.h"
#include "clish/action.h"

typedef struct clish_var_s clish_var_t;

int clish_var_bt_compare(const void *clientnode, const void *clientkey);
void clish_var_bt_getkey(const void *clientnode, lub_bintree_key_t * key);
size_t clish_var_bt_offset(void);
clish_var_t *clish_var_new(const char *name);
void clish_var_delete(clish_var_t *instance);
void clish_var_dump(const clish_var_t *instance);

_CLISH_GET_STR(var, name);
_CLISH_SET(var, bool_t, dynamic);
_CLISH_GET(var, bool_t, dynamic);
_CLISH_SET_STR(var, value);
_CLISH_GET_STR(var, value);
_CLISH_SET_STR(var, saved);
_CLISH_GET_STR(var, saved);
_CLISH_GET(var, clish_action_t *, action);

#endif /* _clish_var_h */
