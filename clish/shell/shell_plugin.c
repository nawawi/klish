/*
 * shell_plugin.c
 */
#include "private.h"
#include <assert.h>
#include <string.h>

#include "lub/string.h"
#include "lub/list.h"
#include "lub/bintree.h"
#include "clish/plugin.h"
#include "clish/view.h"

/*----------------------------------------------------------------------- */
/* For all plugins:
 *  * dlopen(plugin)
 *  * dlsym(initialize function)
 *  * exec init functions to get all plugin syms
 */
int clish_shell_load_plugins(clish_shell_t *this)
{
	lub_list_node_t *iter;
	clish_plugin_t *plugin;

	assert(this);

	/* Iterate elements */
	for(iter = lub_list__get_head(this->plugins);
		iter; iter = lub_list_node__get_next(iter)) {
		plugin = (clish_plugin_t *)lub_list_node__get_data(iter);
		if (!clish_plugin_load(plugin)) {
			fprintf(stderr, "Error: Can't load plugin %s.\n",
				clish_plugin__get_file(plugin));
			return -1;
		}
#ifdef DEBUG
		clish_plugin_dump(plugin);
#endif
	}

	return 0;
}

/*----------------------------------------------------------------------- */
/* Find plugin by name. */
static clish_plugin_t *plugin_by_name(clish_shell_t *this, const char *name)
{
	lub_list_node_t *iter;
	clish_plugin_t *plugin;

	/* Iterate elements */
	for(iter = lub_list__get_head(this->plugins);
		iter; iter = lub_list_node__get_next(iter)) {
		plugin = (clish_plugin_t *)lub_list_node__get_data(iter);
		if (!strcmp(clish_plugin__get_name(plugin), name))
			return plugin;
	}

	return NULL;
}

/*----------------------------------------------------------------------- */
/* Iterate plugins to find symbol by name.
 * The symbol name can be simple or with namespace:
 * mysym@plugin1
 * The symbols with prefix will be resolved using specified plugin only.
 */
static clish_plugin_fn_t *plugins_find_sym(clish_shell_t *this, const char *name)
{
	lub_list_node_t *iter;
	clish_plugin_t *plugin;
	clish_plugin_fn_t *func = NULL;
	/* To parse command name */
	char *saveptr;
	const char *delim = "@";
	char *plugin_name = NULL;
	char *cmdn = NULL;
	char *str = lub_string_dup(name);

	assert(this);

	/* Parse name to get sym name and optional plugin name */
	cmdn = strtok_r(str, delim, &saveptr);
	if (!cmdn)
		goto end;
	plugin_name = strtok_r(NULL, delim, &saveptr);

	if (plugin_name) {
		/* Search for symbol in specified plugin */
		plugin = plugin_by_name(this, plugin_name);
		if (!plugin)
			goto end;
		func = clish_plugin_get_sym(plugin, cmdn);
	} else {
		/* Iterate all plugins */
		for(iter = lub_list__get_head(this->plugins);
			iter; iter = lub_list_node__get_next(iter)) {
			plugin = (clish_plugin_t *)lub_list_node__get_data(iter);
			if ((func = clish_plugin_get_sym(plugin, cmdn)))
				break;
		}
	}
end:
	lub_string_free(str);
	return func;
}

/*----------------------------------------------------------------------- */
static int plugins_link_view(clish_shell_t *this, clish_view_t *view)
{
	clish_command_t *c;
	lub_bintree_iterator_t iter;
	lub_bintree_t *tree;

	tree = clish_view__cmd_tree(view);
	/* Iterate the tree of commands */
	c = lub_bintree_findfirst(tree);
	for (lub_bintree_iterator_init(&iter, tree, c);
		c; c = lub_bintree_iterator_next(&iter)) {
		plugins_find_sym(this, "jjj");
		printf("command: %s\n", clish_command__get_name(c));
	}

	return 0;
}

/*----------------------------------------------------------------------- */
int clish_shell_link_plugins(clish_shell_t *this)
{
	clish_view_t *v;
	lub_bintree_iterator_t iter;

	v = lub_bintree_findfirst(&this->view_tree);
	/* Iterate the tree of views */
	for (lub_bintree_iterator_init(&iter, &this->view_tree, v);
		v; v = lub_bintree_iterator_next(&iter)) {
		if (plugins_link_view(this, v))
			return -1;
	}

	return 0;
}
