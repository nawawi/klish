#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <faux/conv.h>
#include <faux/list.h>
#include <klish/khelper.h>
#include <klish/kcontext.h>
#include <klish/kscheme.h>


struct kcontext_s {
	kcontext_type_e type;
	int retcode;
	const kplugin_t *plugin;
	const ksym_t *sym;
	const kaction_t *action;
	const kcommand_t *command;
};


// Simple methods

// Type
KGET(context, kcontext_type_e, type);
KSET(context, kcontext_type_e, type);

// RetCode
KGET(context, int, retcode);
KSET(context, int, retcode);

// Plugin
KGET(context, const kplugin_t *, plugin);
KSET(context, const kplugin_t *, plugin);

// Sym
KGET(context, const ksym_t *, sym);
KSET(context, const ksym_t *, sym);

// Action
KGET(context, const kaction_t *, action);
KSET(context, const kaction_t *, action);

// Command
KGET(context, const kcommand_t *, command);
KSET(context, const kcommand_t *, command);


kcontext_t *kcontext_new(kcontext_type_e type)
{
	kcontext_t *context = NULL;

	context = faux_zmalloc(sizeof(*context));
	assert(context);
	if (!context)
		return NULL;

	// Initialize
	context->type = type;
	context->plugin = NULL;
	context->sym = NULL;
	context->action = NULL;

	return context;
}


void kcontext_free(kcontext_t *context)
{
	if (!context)
		return;

	faux_free(context);
}
