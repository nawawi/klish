/*
 * action.h
 */

#ifndef _clish_action_h
#define _clish_action_h

typedef struct clish_action_s clish_action_t;

#include "clish/macros.h"
#include "clish/plugin.h"

clish_action_t *clish_action_new(void);
void clish_action_delete(clish_action_t *instance);
void clish_action_dump(const clish_action_t *instance);

_CLISH_SET_STR(action, script);
_CLISH_GET_STR(action, script);
_CLISH_SET(action, const clish_sym_t *, builtin);
_CLISH_GET(action, const clish_sym_t *, builtin);
_CLISH_SET_STR(action, shebang);
_CLISH_GET_STR(action, shebang);
_CLISH_SET(action, bool_t, lock);
_CLISH_GET(action, bool_t, lock);
_CLISH_SET(action, bool_t, interrupt);
_CLISH_GET(action, bool_t, interrupt);
_CLISH_SET(action, bool_t, interactive);
_CLISH_GET(action, bool_t, interactive);

#endif // _clish_action_h
