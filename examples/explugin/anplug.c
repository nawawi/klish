#include <stdlib.h>
#include <stdio.h>
#include <clish/plugin.h>

int anplug_fn(clish_context_t *context, char **out)
{
	printf("anplug: Another plugin\n");
	return 0;
}

int clish_plugin_init(clish_plugin_t *plugin)
{
	clish_plugin_add_sym(plugin, anplug_fn, "an_fn");
}


