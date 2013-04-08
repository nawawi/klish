#include "private.h"

CLISH_PLUGIN_INIT
{
	char *conf = clish_plugin__get_conf(plugin);

	if (conf)
		scripts_path = conf;

	clish_plugin_init_lua((clish_shell_t *)clish_shell);

	clish_plugin__set_name(plugin, "lua_hooks");

	clish_plugin_add_sym(plugin, clish_plugin_lua_action, "hook_action");

	return 0;
}
