#include "private.h"

CLISH_PLUGIN_INIT
{

	clish_plugin_init_lua((clish_shell_t *)clish_shell);

	clish_plugin_add_phook(plugin, clish_plugin_lua_fini, "hook_fini",
			CLISH_SYM_TYPE_FINI);
	clish_plugin_add_sym(plugin, clish_plugin_lua_action, "hook_action");

	return 0;
}
