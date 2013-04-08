#include <lub/string.h>

#include "private.h"

CLISH_PLUGIN_SYM(clish_plugin_lua_action)
{
	clish_context_t *context = (clish_context_t *) clish_context;
	lua_State *L = clish_shell__get_udata(context->shell, LUA_UDATA);

	int res = 0, result = -1, retnum = 0;

	if (!script) /* Nothing to do */
		return (0);

	if (out) {
		*out = NULL;
		retnum = 1;
	}


	if ((res = luaL_loadstring(L, script))) {
		l_print_error(L, __func__, "load", res);
	} else if ((res = lua_pcall(L, 0, retnum, 0))) {
		l_print_error(L, __func__, "exec", res);
	} else {
		if (retnum) {
			if (lua_isstring(L, -1))
				*out = lub_string_dup(lua_tostring(L, -1));
			lua_pop(L, 1);
		}
		result = 0;
	}

	lua_gc(L, LUA_GCCOLLECT, 0);

	return (result);
}
