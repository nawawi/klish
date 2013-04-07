#include <stdlib.h>
#include <stdio.h>
#include <clish/plugin.h>

CLISH_PLUGIN_SYM(anplug_fn)
{
	printf("anplug: Another plugin\n");
	return 0;
}

CLISH_PLUGIN_INIT
{
	char *conf;

	printf("anplug: INIT shell = %p\n", clish_shell);
	/* Set a name of plugin to use in sym@plugin */
	clish_plugin__set_name(plugin, "another_plug");
	/* Add symbols */
	clish_plugin_add_sym(plugin, anplug_fn, "an_fn");
	/* Show plugin config from <PLUGIN>...</PLUGIN> */
	conf = clish_plugin__get_conf(plugin);
	if (conf)
		printf("%s", conf);

	return 0;
}

CLISH_PLUGIN_FINI
{
	printf("anplug: FINI this = %p\n", clish_shell);

	return 0;
}


