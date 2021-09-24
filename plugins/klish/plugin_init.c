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
	kplugin_add_syms(plugin, ksym_new("nop", klish_nop));
	kplugin_add_syms(plugin, ksym_new("tsym", klish_tsym));

	// PTYPEs
	kplugin_add_syms(plugin, ksym_new("COMMAND", klish_ptype_COMMAND));
	kplugin_add_syms(plugin, ksym_new("COMMAND_CASE", klish_ptype_COMMAND_CASE));

	context = context; // Happy compiler

	return 0;
}


int kplugin_klish_fini(kcontext_t *context)
{
//	fprintf(stderr, "Plugin 'klish' fini\n");
	context = context;

	return 0;
}
