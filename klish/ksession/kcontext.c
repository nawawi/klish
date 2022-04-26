#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>

#include <faux/str.h>
#include <faux/conv.h>
#include <faux/list.h>
#include <klish/khelper.h>
#include <klish/kpargv.h>
#include <klish/kcontext.h>
#include <klish/kscheme.h>
#include <klish/ksession.h>
#include <klish/kaction.h>


struct kcontext_s {
	kcontext_type_e type;
	int retcode;
	ksession_t *session;
	kplugin_t *plugin;
	kpargv_t *pargv;
	const kpargv_t *parent_pargv; // Parent
	faux_list_node_t *action_iter; // Current action
	ksym_t *sym;
	int stdin;
	int stdout;
	int stderr;
	pid_t pid;
	bool_t done; // If all actions are done
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

// Pargv
KGET(context, kpargv_t *, pargv);
FAUX_HIDDEN KSET(context, kpargv_t *, pargv);

// Parent pargv
KGET(context, const kpargv_t *, parent_pargv);
FAUX_HIDDEN KSET(context, const kpargv_t *, parent_pargv);

// Action iterator
KGET(context, faux_list_node_t *, action_iter);
FAUX_HIDDEN KSET(context, faux_list_node_t *, action_iter);

// STDIN
KGET(context, int, stdin);
FAUX_HIDDEN KSET(context, int, stdin);

// STDOUT
KGET(context, int, stdout);
FAUX_HIDDEN KSET(context, int, stdout);

// STDERR
KGET(context, int, stderr);
FAUX_HIDDEN KSET(context, int, stderr);

// PID
KGET(context, pid_t, pid);
FAUX_HIDDEN KSET(context, pid_t, pid);

// Session
KGET(context, ksession_t *, session);
FAUX_HIDDEN KSET(context, ksession_t *, session);

// Done
KGET_BOOL(context, done);
FAUX_HIDDEN KSET_BOOL(context, done);


kcontext_t *kcontext_new(kcontext_type_e type)
{
	kcontext_t *context = NULL;

	context = faux_zmalloc(sizeof(*context));
	assert(context);
	if (!context)
		return NULL;

	// Initialize
	context->type = type;
	context->retcode = 0;
	context->plugin = NULL;
	context->pargv = NULL;
	context->parent_pargv = NULL; // Don't free
	context->action_iter = NULL;
	context->sym = NULL;
	context->stdin = -1;
	context->stdout = -1;
	context->stderr = -1;
	context->pid = -1; // PID of currently executed ACTION
	context->session = NULL; // Don't free
	context->done = BOOL_FALSE;

	return context;
}


void kcontext_free(kcontext_t *context)
{
	if (!context)
		return;

	kpargv_free(context->pargv);

	if (context->stdin != -1)
		close(context->stdin);
	if (context->stdout != -1)
		close(context->stdout);
	if (context->stderr != -1)
		close(context->stderr);

	faux_free(context);
}


kparg_t *kcontext_candidate_parg(const kcontext_t *context)
{
	assert(context);
	if (!context)
		return NULL;

	return kpargv_candidate_parg(kcontext_parent_pargv(context));
}


const kentry_t *kcontext_candidate_entry(const kcontext_t *context)
{
	assert(context);
	if (!context)
		return NULL;

	return kparg_entry(kcontext_candidate_parg(context));
}


const char *kcontext_candidate_value(const kcontext_t *context)
{
	assert(context);
	if (!context)
		return NULL;

	return kparg_value(kcontext_candidate_parg(context));
}


const kaction_t *kcontext_action(const kcontext_t *context)
{
	faux_list_node_t *node = NULL;

	assert(context);
	if (!context)
		return NULL;

	node = kcontext_action_iter(context);
	if (!node)
		return NULL;

	return (const kaction_t *)faux_list_data(node);
}


const char *kcontext_script(const kcontext_t *context)
{
	const kaction_t *action = NULL;

	assert(context);
	if (!context)
		return NULL;

	action = kcontext_action(context);
	if (!action)
		return NULL;

	return kaction_script(action);
}
