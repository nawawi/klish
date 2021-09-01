/** @file kexec.c
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <faux/list.h>
#include <klish/khelper.h>
#include <klish/kcontext.h>
#include <klish/kexec.h>

struct kexec_s {
	faux_list_t *contexts;
	int stdin;
	int stdout;
	int stderr;
};


// STDIN
KGET(exec, int, stdin);
KSET(exec, int, stdin);

// STDOUT
KGET(exec, int, stdout);
KSET(exec, int, stdout);

// STDERR
KGET(exec, int, stderr);
KSET(exec, int, stderr);

// CONTEXT list
KADD_NESTED(exec, kcontext_t *, contexts);
KNESTED_LEN(exec, contexts);
KNESTED_IS_EMPTY(exec, contexts);
KNESTED_ITER(exec, contexts);
KNESTED_EACH(exec, kcontext_t *, contexts);


kexec_t *kexec_new()
{
	kexec_t *exec = NULL;

	exec = faux_zmalloc(sizeof(*exec));
	assert(exec);
	if (!exec)
		return NULL;

	// List of execute contexts
	exec->contexts = faux_list_new(FAUX_LIST_UNSORTED, FAUX_LIST_NONUNIQUE,
		NULL, NULL, (void (*)(void *))kcontext_free);
	assert(exec->contexts);

	// I/O
	exec->stdin = -1;
	exec->stdout = -1;
	exec->stderr = -1;

	return exec;
}


void kexec_free(kexec_t *exec)
{
	if (!exec)
		return;

	faux_list_free(exec->contexts);

	free(exec);
}


size_t kexec_len(const kexec_t *exec)
{
	assert(exec);
	if (!exec)
		return 0;

	return faux_list_len(exec->contexts);
}


size_t kexec_is_empty(const kexec_t *exec)
{
	assert(exec);
	if (!exec)
		return 0;

	return faux_list_is_empty(exec->contexts);
}


bool_t kexec_add(kexec_t *exec, kcontext_t *context)
{
	assert(exec);
	assert(context);
	if (!exec)
		return BOOL_FALSE;
	if (!context)
		return BOOL_FALSE;

	if (!faux_list_add(exec->contexts, context))
		return BOOL_FALSE;

	return BOOL_TRUE;
}



static bool_t kexec_prepare(kexec_t *exec)
{
	int pipefd[2] = {};
	faux_list_node_t *iter = NULL;
	int global_stderr = -1;

	assert(exec);
	if (!exec)
		return BOOL_FALSE;
	// Nothing to prepare for empty list
	if (kexec_contexts_is_empty(exec))
		return BOOL_FALSE;

	// Create "global" stdin, stdout, stderr for the whole job execution.
	// Now function creates only the simple pipes but somedays it will be
	// able to create pseudo-terminal for interactive sessions.

	// STDIN
	if (pipe(pipefd) < 0)
		return BOOL_FALSE;
	kcontext_set_stdin(faux_list_data(faux_list_head(exec->contexts)),
		pipefd[0]); // Read end
	kexec_set_stdin(exec, pipefd[1]); // Write end

	// STDOUT
	if (pipe(pipefd) < 0)
		return BOOL_FALSE;
	kexec_set_stdout(exec, pipefd[0]); // Read end
	kcontext_set_stdout(faux_list_data(faux_list_tail(exec->contexts)),
		pipefd[1]); // Write end

	// STDERR
	if (pipe(pipefd) < 0)
		return BOOL_FALSE;
	kexec_set_stderr(exec, pipefd[0]); // Read end
	// STDERR write end will be set to all list members as stderr
	global_stderr = pipefd[1]; // Write end

	// Iterate all context_t elements to fill all stdin, stdout, stderr
	for (iter = faux_list_head(exec->contexts); iter;
		iter = faux_list_next_node(iter)) {
		faux_list_node_t *next = faux_list_next_node(iter);
		kcontext_t *context = (kcontext_t *)faux_list_data(iter);

		// Set the same STDERR to all contexts
		kcontext_set_stderr(context, global_stderr);

		// Create pipes beetween processes
		if (next) {
			kcontext_t *next_context = (kcontext_t *)faux_list_data(next);
			if (pipe(pipefd) < 0)
				return BOOL_FALSE;
			kcontext_set_stdout(context, pipefd[1]); // Write end
			kcontext_set_stdin(next_context, pipefd[0]); // Read end
		}
	}

	return BOOL_TRUE;
}


static bool_t exec_action(kcontext_t *context, const kaction_t *action)
{
	context = context;
	action = action;

	return BOOL_TRUE;
}


static bool_t exec_action_sequence(kcontext_t *context, pid_t pid, int wstatus)
{
	faux_list_node_t *iter = NULL;
	int exitstatus = WEXITSTATUS(wstatus);

	assert(context);
	if (!context)
		return BOOL_FALSE;

	// There is two reasons to don't start any real actions.
	// - The ACTION sequence is already done;
	// - Passed PID (PID of completed process) is not owned by this context.
	// Returns true because it's not an error.
	if (kcontext_done(context) || (kcontext_pid(context) != pid))
		return BOOL_TRUE;

	iter = kcontext_action_iter(context); // Get saved current ACTION
	do {
		faux_list_t *actions = NULL;
		const kaction_t *action = NULL;
		// Here we know that given PID is our PID and some ACTIONs are
		// left to be executed.

		// If some process returns then compute current retcode.
		if (iter) {
			const kaction_t *terminated_action = NULL;
			terminated_action = faux_list_data(iter);
			assert(terminated_action);
			if (kaction_update_retcode(terminated_action))
				kcontext_set_retcode(context, exitstatus);
		}

	if (!iter) { // Is it the first ACTION within list
		actions = kentry_actions(kpargv_command(kcontext_pargv(context)));
		assert(actions);
		iter = faux_list_head(actions);
	} else {
		iter = faux_list_next_node(iter);
	}
	kcontext_set_action_iter(context, iter);

	if (!iter) { // It was last ACTION
		kcontext_set_done(context, BOOL_TRUE);
	}

	action = (const kaction_t *)faux_list_data(iter);
	assert(action);
	exec_action(context, action);

printf("CONTEXT\n");

	} while (iter);

	wstatus = wstatus;

	return BOOL_TRUE;
}


static bool_t continue_command_execution(kexec_t *exec, pid_t pid, int wstatus)
{
	faux_list_node_t *iter = NULL;
	kcontext_t *context = NULL;

	assert(exec);
	if (!exec)
		return BOOL_FALSE;

	iter = kexec_contexts_iter(exec);
	while ((context = kexec_contexts_each(&iter))) {
		exec_action_sequence(context, pid, wstatus);
	}

	return BOOL_TRUE;
}


bool_t kexec_exec(kexec_t *exec)
{
	assert(exec);
	if (!exec)
		return BOOL_FALSE;

	// Firsly prepare kexec object for execution. The file streams must
	// be created for stdin, stdout, stderr of processes.
	if (!kexec_prepare(exec))
		return BOOL_FALSE;

	// Here no ACTIONs are executing, so pass -1 as pid of terminated
	// ACTION's process.
	continue_command_execution(exec, -1, 0);

	return BOOL_TRUE;
}
