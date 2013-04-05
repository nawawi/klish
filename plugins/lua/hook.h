#ifndef _HOOK_H_
#define _HOOK_H_

#include <clish/shell.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#define LUA_UDATA	"lua state"

void l_print_error(lua_State *, const char *, const char *, int);

#include "hook_spec.h"

#endif /* _HOOK_H_ */
