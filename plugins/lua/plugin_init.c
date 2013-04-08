#include "private.h"

CLISH_PLUGIN_INIT
{
<<<<<<< HEAD
	char *conf = clish_plugin__get_conf(plugin);

	if (conf)
		scripts_path = conf;

	clish_plugin_init_lua((clish_shell_t *)clish_shell);

	clish_plugin__set_name(plugin, "lua_hooks");

=======

	clish_plugin_init_lua((clish_shell_t *)clish_shell);

	clish_plugin_add_phook(plugin, clish_plugin_lua_fini, "hook_fini",
			CLISH_SYM_TYPE_FINI);
>>>>>>> b9e9072034ce6f652f230161cea580b52cf5a9b9
	clish_plugin_add_sym(plugin, clish_plugin_lua_action, "hook_action");

	return 0;
}
