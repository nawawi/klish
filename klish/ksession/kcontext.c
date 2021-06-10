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
	kplugin_t *plugin;
	kcommand_t *command;
	kaction_t *action;
	ksym_t *sym;
	int stdin;
	int stdout;
	int stderr;
};


// Simple methods

// Type
KGET(context, kcontext_type_e, type);
FAUX_HIDDEN KSET(context, kcontext_type_e, type);

// RetCode
KGET(context, int, retcode);
FAUX_HIDDEN KSET(context, int, retcode);

// Plugin
KGET(context, kplugin_t *, plugin);
FAUX_HIDDEN KSET(context, kplugin_t *, plugin);

// Sym
KGET(context, ksym_t *, sym);
FAUX_HIDDEN KSET(context, ksym_t *, sym);

// Action
KGET(context, kaction_t *, action);
FAUX_HIDDEN KSET(context, kaction_t *, action);

// Command
KGET(context, kcommand_t *, command);
FAUX_HIDDEN KSET(context, kcommand_t *, command);

// STDIN
KGET(context, int, stdin);
FAUX_HIDDEN KSET(context, int, stdin);

// STDOUT
KGET(context, int, stdout);
FAUX_HIDDEN KSET(context, int, stdout);

// STDERR
KGET(context, int, stderr);
FAUX_HIDDEN KSET(context, int, stderr);


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
	context->command = NULL;
	context->action = NULL;
	context->sym = NULL;

	// I/O
	context->stdin = -1;
	context->stdout = -1;
	context->stderr = -1;

	return context;
}


void kcontext_free(kcontext_t *context)
{
	if (!context)
		return;

	faux_free(context);
}
