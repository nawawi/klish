#include <stdlib.h>

#include "hook.h"

CLISH_HOOK_FINI(clish_plugin_hook_fini)
{
	clish_context_t *context = (clish_context_t *) clish_context;
	lua_State *L = clish_shell__del_udata(context->shell, LUA_UDATA);

	lua_close(L);

	return (0);
}
