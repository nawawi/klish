#if !defined LUA_VERSION_NUM || LUA_VERSION_NUM==501
#ifndef luaL_newlib
#define luaL_newlib(L, l) \
  (lua_newtable((L)),luaL_setfuncs((L), (l), 0))
#endif
extern void luaL_setfuncs(lua_State *L, const luaL_Reg *l, int nup);
extern void luaL_requiref(lua_State *L, char const* modname,
	      lua_CFunction openf, int glb);
#endif
