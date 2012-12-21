#include <stdlib.h>
#include <stdio.h>
#include <clish/plugin.h>

int explugin_fn(void *context, char **out)
{
	printf("explugin: Hello world\n");
	return 0;
}

int clish_plugin_init(clish_plugin_t *plugin)
{
	clish_plugin_add_sym(plugin, explugin_fn, "hello");
}


