/*
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <faux/faux.h>
#include <klish/kplugin.h>
#include <klish/kcontext.h>

//#include "private.h"


const uint8_t kplugin_klish_major = KPLUGIN_MAJOR;
const uint8_t kplugin_klish_minor = KPLUGIN_MINOR;


int kplugin_klish_init(kcontext_t *context)
{
	/* Add hooks */
/*	clish_plugin_add_phook(plugin, clish_hook_access,
		"clish_hook_access", CLISH_SYM_TYPE_ACCESS);
	clish_plugin_add_phook(plugin, clish_hook_config,
		"clish_hook_config", CLISH_SYM_TYPE_CONFIG);
	clish_plugin_add_phook(plugin, clish_hook_log,
		"clish_hook_log", CLISH_SYM_TYPE_LOG);
*/

	/* Add builtin syms */
/*	clish_plugin_add_psym(plugin, clish_close, "clish_close");
	clish_plugin_add_psym(plugin, clish_overview, "clish_overview");
	clish_plugin_add_psym(plugin, clish_source, "clish_source");
	clish_plugin_add_psym(plugin, clish_source_nostop, "clish_source_nostop");
	clish_plugin_add_psym(plugin, clish_history, "clish_history");
	clish_plugin_add_psym(plugin, clish_nested_up, "clish_nested_up");
	clish_plugin_add_psym(plugin, clish_nop, "clish_nop");
	clish_plugin_add_psym(plugin, clish_wdog, "clish_wdog");
	clish_plugin_add_psym(plugin, clish_macros, "clish_macros");
	clish_plugin_add_osym(plugin, clish_script, "clish_script");
	clish_plugin_add_psym(plugin, clish_machine_interface, "clish_machine_interface");
	clish_plugin_add_psym(plugin, clish_human_interface, "clish_human_interface");
	clish_plugin_add_psym(plugin, clish_print_script, "clish_print_script");
	clish_plugin_add_psym(plugin, clish_print_var, "clish_print_var");
*/
	//fprintf(stderr, "Plugin 'klish' init\n");
	context = context; // Happy compiler

	return 0;
}


int kplugin_klish_fini(kcontext_t *context)
{
	//fprintf(stderr, "Plugin 'klish' fini\n");
	context = context;

	return 0;
}
