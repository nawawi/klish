/*
 * shell_plugin.c
 */
#include "private.h"
#include <assert.h>

#include "lub/list.h"
#include "clish/plugin.h"

/*----------------------------------------------------------------------- */
int clish_shell_load_plugins(clish_shell_t *this)
{
	lub_list_node_t *iter;
	clish_plugin_t *plugin;

	assert(this);

	/* Iterate elements */
	for(iter = lub_list__get_head(this->plugins);
		iter; iter = lub_list_node__get_next(iter)) {
		plugin = (clish_plugin_t *)lub_list_node__get_data(iter);
		if (!clish_plugin_load(plugin))
			return -1;
		clish_plugin_dump(plugin);
	}

	return 0;
}
