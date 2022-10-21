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


const uint8_t kplugin_script_major = KPLUGIN_MAJOR;
const uint8_t kplugin_script_minor = KPLUGIN_MINOR;


int kplugin_script_init(kcontext_t *context)
{
	kplugin_t *plugin = NULL;
	ksym_t *sym = NULL;

	assert(context);
	plugin = kcontext_plugin(context);
	assert(plugin);

	kplugin_add_syms(plugin, ksym_new("script", script_script));

	return 0;
}


int kplugin_script_fini(kcontext_t *context)
{
//	fprintf(stderr, "Plugin 'script' fini\n");
	context = context;

	return 0;
}
