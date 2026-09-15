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
#include <klish/kscheme.h>
#include <klish/kexec.h>


struct kcontext_s {
	kcontext_type_e type;
	kscheme_t *scheme;
	int retcode;
	ksession_t *session;
	kplugin_t *plugin;
	kpargv_t *pargv;
	const kpargv_t *parent_pargv; // Parent
	const kcontext_t *parent_context; // Parent context (if available)
	const kexec_t *parent_exec; // Parent exec (if available)
	faux_list_node_t *action_iter; // Current action
	ksym_t *sym;
	int std_in;
	int std_out;
	int std_err;
	faux_buf_t *bufout; // Don't free. Just a link
	faux_buf_t *buferr; // Don't free. Just a link
	pid_t pid;
	bool_t done; // If all actions are done
	char *line; // Text command context belong to
	size_t pipeline_stage; // Index of current command within full pipeline
	bool_t is_last_pipeline_stage;
};


// Simple methods

// Type
KGET(context, kcontext_type_e, type);
FAUX_HIDDEN KSET(context, kcontext_type_e, type);

// Scheme
KGET(context, kscheme_t *, scheme);
KSET(context, kscheme_t *, scheme);

// RetCode
KGET(context, int, retcode);
FAUX_HIDDEN KSET(context, int, retcode);

// Plugin
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

// Parent context
KGET(context, const kcontext_t *, parent_context);
FAUX_HIDDEN KSET(context, const kcontext_t *, parent_context);

// Parent exec
KGET(context, const kexec_t *, parent_exec);
FAUX_HIDDEN KSET(context, const kexec_t *, parent_exec);

// Action iterator
KGET(context, faux_list_node_t *, action_iter);
FAUX_HIDDEN KSET(context, faux_list_node_t *, action_iter);

// STDIN
KGET(context, int, std_in);
FAUX_HIDDEN KSET(context, int, std_in);

// STDOUT
KGET(context, int, std_out);
FAUX_HIDDEN KSET(context, int, std_out);

// STDERR
KGET(context, int, std_err);
FAUX_HIDDEN KSET(context, int, std_err);

// bufout
KGET(context, faux_buf_t *, bufout);
FAUX_HIDDEN KSET(context, faux_buf_t *, bufout);

// buferr
KGET(context, faux_buf_t *, buferr);
FAUX_HIDDEN KSET(context, faux_buf_t *, buferr);

// PID
KGET(context, pid_t, pid);
FAUX_HIDDEN KSET(context, pid_t, pid);

// Session
KGET(context, ksession_t *, session);
FAUX_HIDDEN KSET(context, ksession_t *, session);

// Done
KGET_BOOL(context, done);
FAUX_HIDDEN KSET_BOOL(context, done);

// Line
KGET_STR(context, line);
FAUX_HIDDEN KSET_STR(context, line);

// Pipeline stage
KGET(context, size_t, pipeline_stage);
FAUX_HIDDEN KSET(context, size_t, pipeline_stage);

// Is last pipeline stage
KGET(context, bool_t, is_last_pipeline_stage);
FAUX_HIDDEN KSET(context, bool_t, is_last_pipeline_stage);


kcontext_t *kcontext_new(kcontext_type_e type)
{
	kcontext_t *context = NULL;

	context = faux_zmalloc(sizeof(*context));
	assert(context);
	if (!context)
		return NULL;

	// Initialize
	context->type = type;
	context->scheme = NULL;
	context->retcode = 0;
	context->plugin = NULL;
	context->pargv = NULL;
	context->parent_pargv = NULL; // Don't free
	context->parent_context = NULL; // Don't free
	context->parent_exec = NULL; // Don't free
	context->action_iter = NULL;
	context->sym = NULL;
	context->std_in = -1;
	context->std_out = -1;
	context->std_err = -1;
	context->bufout = NULL;
	context->buferr = NULL;
	context->pid = -1; // PID of currently executed ACTION
	context->session = NULL; // Don't free
	context->done = BOOL_FALSE;
	context->line = NULL;
	context->pipeline_stage = 0;
	context->is_last_pipeline_stage = BOOL_TRUE;

	return context;
}


void kcontext_free(kcontext_t *context)
{
	if (!context)
		return;

	kpargv_free(context->pargv);

	if (context->std_in != -1)
		close(context->std_in);
	if (context->std_out != -1)
		close(context->std_out);
	if (context->std_err != -1)
		close(context->std_err);

	faux_str_free(context->line);

	faux_free(context);
}


kparg_t *kcontext_candidate_parg(const kcontext_t *context)
{
	const kpargv_t *pargv = NULL;

	assert(context);
	if (!context)
		return NULL;
	pargv = kcontext_parent_pargv(context);
	if (!pargv)
		return NULL;

	return kpargv_candidate_parg(pargv);
}


kentry_t *kcontext_candidate_entry(const kcontext_t *context)
{
	kparg_t *parg = NULL;

	assert(context);
	if (!context)
		return NULL;
	parg = kcontext_candidate_parg(context);
	if (!parg)
		return NULL;

	return kparg_entry(parg);
}


const char *kcontext_candidate_value(const kcontext_t *context)
{
	kparg_t *parg = NULL;

	assert(context);
	if (!context)
		return NULL;
	parg = kcontext_candidate_parg(context);
	if (!parg)
		return NULL;

	return kparg_value(parg);
}


kaction_t *kcontext_action(const kcontext_t *context)
{
	faux_list_node_t *node = NULL;

	assert(context);
	if (!context)
		return NULL;

	node = kcontext_action_iter(context);
	if (!node)
		return NULL;

	return (kaction_t *)faux_list_data(node);
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


bool_t kcontext_named_udata_new(kcontext_t *context,
	const char *name, void *data, kudata_data_free_fn free_fn)
{
	assert(context);
	if (!context)
		return BOOL_FALSE;

	return kscheme_named_udata_new(context->scheme, name, data, free_fn);
}


void *kcontext_named_udata(const kcontext_t *context, const char *name)
{
	assert(context);
	if (!context)
		return NULL;

	return kscheme_named_udata(context->scheme, name);
}


void *kcontext_udata(const kcontext_t *context)
{
	kplugin_t *plugin = NULL;

	assert(context);
	if (!context)
		return NULL;

	plugin = kcontext_plugin(context);
	if (!plugin)
		return NULL;

	return kplugin_udata(plugin);
}


kentry_t *kcontext_command(const kcontext_t *context)
{
	kpargv_t *pargv = NULL;

	assert(context);
	if (!context)
		return NULL;
	pargv = kcontext_pargv(context);
	if (!pargv)
		return NULL;

	return kpargv_command(pargv);
}


kplugin_t *kcontext_plugin(const kcontext_t *context)
{
	const kaction_t *action = NULL;

	assert(context);
	if (!context)
		return NULL;

	// If plugin field is specified then return it. It is specified for
	// plugin's init() and fini() functions.
	if (context->plugin)
		return context->plugin;

	// If plugin is not explicitly specified then return parent plugin for
	// currently executed sym (ACTION structure contains it).
	action = kcontext_action(context);
	if (!action)
		return NULL;

	return kaction_plugin(action);
}


ssize_t kcontext_fwrite(const kcontext_t *context, FILE *stream,
	const char *line, size_t len)
{
	const kaction_t *action = NULL;
	const ksym_t *sym = NULL;
	faux_buf_t *buf = NULL;
	int rc = -1;

	if (len < 1)
		return 0;
	if (!context)
		return -1;
	if (stream != stdout && stream != stderr)
		return -1;
	action = kcontext_action(context);
	if (!action)
		return -1;
	sym = kaction_sym(action);
	if (!sym)
		return -1;

	// "Silent" output
	if (ksym_sync(sym) && ksym_silent(sym) &&
		kcontext_is_last_pipeline_stage(context))
	{
		buf = (stream == stderr) ? kcontext_buferr(context) :
			kcontext_bufout(context);
		if (!buf)
			return -1;
		faux_buf_write(buf, line, len);
		return len;
	}

	rc = fwrite(line, 1, len, stream);
	fflush(stream);

	return rc;
}


static int kcontext_vprintf(const kcontext_t *context,
	FILE *stream, const char *fmt, va_list ap)
{
	int rc = -1;
	char *line = NULL;

	line = faux_str_vsprintf(fmt, ap);
	rc = kcontext_fwrite(context, stream, line, strlen(line));
	faux_str_free(line);

	return rc;
}


int kcontext_fprintf(const kcontext_t *context, FILE *stream,
	const char *fmt, ...)
{
	int rc = -1;
	va_list ap;

	va_start(ap, fmt);
	rc = kcontext_vprintf(context, stream, fmt, ap);
	va_end(ap);

	return rc;
}


int kcontext_printf(const kcontext_t *context, const char *fmt, ...)
{
	int rc = -1;
	va_list ap;

	va_start(ap, fmt);
	rc = kcontext_vprintf(context, stdout, fmt, ap);
	va_end(ap);

	return rc;
}
