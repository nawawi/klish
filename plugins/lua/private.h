#ifndef _PLUGIN_H_
#define _PLUGIN_H_

#include <clish/shell.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#define LUA_UDATA	"lua state"

<<<<<<< HEAD
extern char *scripts_path;
void l_print_error(lua_State *, const char *, const char *, int);
int clish_plugin_init_lua(clish_shell_t *shell);

=======
void l_print_error(lua_State *, const char *, const char *, int);
int clish_plugin_init_lua(clish_shell_t *shell);

CLISH_HOOK_FINI(clish_plugin_lua_fini);
>>>>>>> b9e9072034ce6f652f230161cea580b52cf5a9b9
CLISH_PLUGIN_SYM(clish_plugin_lua_action);

#endif /* _PLUGIN_H_ */
