if WITH_LUA
plugin_LTLIBRARIES += kplugin-lua.la
endif

kplugin_lua_la_SOURCES =
kplugin_lua_la_LDFLAGS = $(AM_LDFLAGS) -avoid-version -module
kplugin_lua_la_LIBS =
kplugin_lua_la_CFLAGS = $(AM_LDFLAGS) @LUA_INCLUDE@
kplugin_lua_la_LIBADD = libklish.la @LUA_LIB@

kplugin_lua_la_SOURCES += \
	plugins/lua/klish_lua.c plugins/lua/lua-compat.c
