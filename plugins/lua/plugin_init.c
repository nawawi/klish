#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "private.h"

static char * trim(char *str)
{
	size_t len = 0;
	const char *first = str;
	char *last = str + strlen(str) - 1;
	char *new = NULL;

	while (first < last && isspace(*first))
		++first;
	while (first < last && isspace(*last))
		--last;

	len = last - first + 1;
	new = malloc(len + 1);
	memcpy(new, first, len);
	new[len] = '\0';

	return new;
}

CLISH_PLUGIN_INIT
{
	char *conf = clish_plugin__get_conf(plugin);

	if (conf) {
		scripts_path = trim(conf);
	}

	if(clish_plugin_init_lua(clish_shell))
		return (-1);

	clish_plugin__set_name(plugin, "lua_hooks");

	clish_plugin_add_sym(plugin, clish_plugin_lua_action, "hook_action");

	return 0;
}
