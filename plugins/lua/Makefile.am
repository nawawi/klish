if WITH_LUA
lib_LTLIBRARIES += libklish-plugin-lua.la
endif

libklish_plugin_lua_la_SOURCES =
libklish_plugin_lua_la_LDFLAGS = $(AM_LDFLAGS)
libklish_plugin_lua_la_LIBS =
libklish_plugin_lua_la_CFLAGS = $(AM_LDFLAGS) @LUA_INCLUDE@
libklish_plugin_lua_la_LIBADD = libklish.la @LUA_LIB@

libklish_plugin_lua_la_SOURCES += \
	plugins/lua/klish_lua.c plugins/lua/lua-compat.c
