#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#if !defined LUA_VERSION_NUM || LUA_VERSION_NUM==501
void luaL_setfuncs(lua_State *L, const luaL_Reg *l, int nup)
{
	luaL_checkstack(L, nup+1, "too many upvalues");
	for (; l->name != NULL; l++) {  /* fill the table with given functions */
		int i;
		lua_pushstring(L, l->name);
		for (i = 0; i < nup; i++)  /* copy upvalues to the top */
			lua_pushvalue(L, -(nup+1));
		lua_pushcclosure(L, l->func, nup);  /* closure with those upvalues */
		lua_settable(L, -(nup + 3));
	}
	lua_pop(L, nup);  /* remove upvalues */
}


void luaL_requiref(lua_State *L, char const* modname,
                    lua_CFunction openf, int glb)
{
	luaL_checkstack(L, 3, "not enough stack slots");
	lua_pushcfunction(L, openf);
	lua_pushstring(L, modname);
	lua_call(L, 1, 1);
	lua_getglobal(L, "package");
	lua_getfield(L, -1, "loaded");
	lua_replace(L, -2);
	lua_pushvalue(L, -2);
	lua_setfield(L, -2, modname);
	lua_pop(L, 1);
	if (glb) {
		lua_pushvalue(L, -1);
		lua_setglobal(L, modname);
	}
}
#endif
