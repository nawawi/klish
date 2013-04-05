#include "hook.h"

CLISH_PLUGIN_INIT
{
	clish_plugin_add_phook(plugin, clish_plugin_hook_init, "hook_init",
			CLISH_SYM_TYPE_INIT);
	clish_plugin_add_phook(plugin, clish_plugin_hook_fini, "hook_fini",
			CLISH_SYM_TYPE_FINI);
	clish_plugin_add_phook(plugin, clish_plugin_hook_action, "hook_action",
			CLISH_SYM_TYPE_ACTION);

	return 0;
}
