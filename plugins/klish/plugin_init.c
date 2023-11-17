/*
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include <faux/faux.h>
#include <klish/kplugin.h>
#include <klish/kcontext.h>

#include "private.h"


const uint8_t kplugin_klish_major = KPLUGIN_MAJOR;
const uint8_t kplugin_klish_minor = KPLUGIN_MINOR;


int kplugin_klish_init(kcontext_t *context)
{
	kplugin_t *plugin = NULL;

	assert(context);
	plugin = kcontext_plugin(context);
	assert(plugin);

	// Misc
	kplugin_add_syms(plugin, ksym_new_ext("nop", klish_nop,
		KSYM_USERDEFINED_PERMANENT, KSYM_SYNC));
	kplugin_add_syms(plugin, ksym_new("tsym", klish_tsym));
	kplugin_add_syms(plugin, ksym_new("print", klish_print));
	kplugin_add_syms(plugin, ksym_new("printl", klish_printl));
	kplugin_add_syms(plugin, ksym_new_ext("pwd", klish_pwd,
		KSYM_PERMANENT, KSYM_SYNC));
	kplugin_add_syms(plugin, ksym_new("prompt", klish_prompt));

	// Log
	kplugin_add_syms(plugin, ksym_new("syslog", klish_syslog));

	// Navigation
	// Navigation must be permanent (no dry-run) and sync. Because unsync
	// actions will be fork()-ed so it can't change current path.
	kplugin_add_syms(plugin, ksym_new_ext("nav", klish_nav,
		KSYM_PERMANENT, KSYM_SYNC));

	// PTYPEs
	// These PTYPEs are simple and fast so set SYNC flag
	kplugin_add_syms(plugin, ksym_new_ext("COMMAND", klish_ptype_COMMAND,
		KSYM_USERDEFINED_PERMANENT, KSYM_SYNC));
	kplugin_add_syms(plugin, ksym_new_ext("completion_COMMAND", klish_completion_COMMAND,
		KSYM_USERDEFINED_PERMANENT, KSYM_SYNC));
	kplugin_add_syms(plugin, ksym_new_ext("help_COMMAND", klish_help_COMMAND,
		KSYM_USERDEFINED_PERMANENT, KSYM_SYNC));
	kplugin_add_syms(plugin, ksym_new_ext("COMMAND_CASE", klish_ptype_COMMAND_CASE,
		KSYM_USERDEFINED_PERMANENT, KSYM_SYNC));
	kplugin_add_syms(plugin, ksym_new_ext("INT", klish_ptype_INT,
		KSYM_USERDEFINED_PERMANENT, KSYM_SYNC));
	kplugin_add_syms(plugin, ksym_new_ext("UINT", klish_ptype_UINT,
		KSYM_USERDEFINED_PERMANENT, KSYM_SYNC));
	kplugin_add_syms(plugin, ksym_new_ext("STRING", klish_ptype_STRING,
		KSYM_USERDEFINED_PERMANENT, KSYM_SYNC));

	return 0;
}


int kplugin_klish_fini(kcontext_t *context)
{
//	fprintf(stderr, "Plugin 'klish' fini\n");
	context = context;

	return 0;
}
