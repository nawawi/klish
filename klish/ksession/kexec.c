/** @file kexec.c
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include <faux/list.h>
#include <faux/buf.h>
#include <klish/khelper.h>
#include <klish/kcontext.h>
#include <klish/kexec.h>

struct kexec_s {
	faux_list_t *contexts;
	bool_t dry_run;
	int stdin;
	int stdout;
	int stderr;
	faux_buf_t *bufin;
	faux_buf_t *bufout;
	faux_buf_t *buferr;
};

// Dry-run
KGET_BOOL(exec, dry_run);
KSET_BOOL(exec, dry_run);

// STDIN
KGET(exec, int, stdin);
KSET(exec, int, stdin);

// STDOUT
KGET(exec, int, stdout);
KSET(exec, int, stdout);

// STDERR
KGET(exec, int, stderr);
KSET(exec, int, stderr);

// BufIN
KGET(exec, faux_buf_t *, bufin);
KSET(exec, faux_buf_t *, bufin);

// BufOUT
KGET(exec, faux_buf_t *, bufout);
KSET(exec, faux_buf_t *, bufout);

// BufERR
KGET(exec, faux_buf_t *, buferr);
KSET(exec, faux_buf_t *, buferr);

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

	exec->dry_run = BOOL_FALSE;

	// List of execute contexts
	exec->contexts = faux_list_new(FAUX_LIST_UNSORTED, FAUX_LIST_NONUNIQUE,
		NULL, NULL, (void (*)(void *))kcontext_free);
	assert(exec->contexts);

	// I/O
	exec->stdin = -1;
	exec->stdout = -1;
	exec->stderr = -1;

	exec->bufin = faux_buf_new(0);
	exec->bufout = faux_buf_new(0);
	exec->buferr = faux_buf_new(0);

	return exec;
}


void kexec_free(kexec_t *exec)
{
	if (!exec)
		return;

	faux_list_free(exec->contexts);

	faux_buf_free(exec->bufin);
	faux_buf_free(exec->bufout);
	faux_buf_free(exec->buferr);

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


// kexec is done when all the kexec's contexts are done
bool_t kexec_done(const kexec_t *exec)
{
	faux_list_node_t *iter = NULL;
	kcontext_t *context = NULL;

	assert(exec);
	if (!exec)
		return BOOL_FALSE;

	iter = kexec_contexts_iter(exec);
	while ((context = kexec_contexts_each(&iter))) {
		if (!kcontext_done(context))
			return BOOL_FALSE;
	}

	return BOOL_TRUE;
}


// Retcode of kexec is a retcode of its first context execution because
// next contexts just a filters. Retcode valid if kexec is done. Else current
// retcode is non-valid and will not be returned at all.
bool_t kexec_retcode(const kexec_t *exec, int *status)
{
	assert(exec);
	if (!exec)
		return BOOL_FALSE;

	if (kexec_is_empty(exec))
		return BOOL_FALSE;

	if (!kexec_done(exec)) // Unfinished execution
		return BOOL_FALSE;

	if (status)
		*status = kcontext_retcode(
			(kcontext_t *)faux_list_data(faux_list_head(exec->contexts)));

	return BOOL_TRUE;
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
	int fflags = 0;

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
	// Read end of 'stdout' pipe must be non-blocked
	fflags = fcntl(pipefd[0], F_GETFL);
	fcntl(pipefd[0], F_SETFL, fflags | O_NONBLOCK);
	kexec_set_stdout(exec, pipefd[0]); // Read end
	kcontext_set_stdout(faux_list_data(faux_list_tail(exec->contexts)),
		pipefd[1]); // Write end

	// STDERR
	if (pipe(pipefd) < 0)
		return BOOL_FALSE;
	// Read end of 'stderr' pipe must be non-blocked
	fflags = fcntl(pipefd[0], F_GETFL);
	fcntl(pipefd[0], F_SETFL, fflags | O_NONBLOCK);
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


// === SYNC symbol execution
// The function will be executed right here. It's necessary for
// navigation implementation for example. To grab function output the
// service process will be forked. It gets output and stores it to the
// internal buffer. After sym function return grabber will write
// buffered data back. So grabber will simulate async sym execution.
static bool_t exec_action_sync(kcontext_t *context, const kaction_t *action,
	pid_t *pid, int *retcode)
{
	ksym_fn fn = NULL;
	int exitcode = 0;
	pid_t child_pid = -1;
	int pipefd[2] = {};
//	int fflags = 0;

	// Create pipe beetween sym function and grabber
	if (pipe(pipefd) < 0)
		return BOOL_FALSE;

	fn = ksym_function(kaction_sym(action));

	fflush(stdout);
	fflush(stderr);

	// Fork the grabber
	child_pid = fork();
	if (child_pid == -1) {
		close(pipefd[0]);
		close(pipefd[1]);
		return BOOL_FALSE;
	}

	// Parent
	if (child_pid != 0) {
		int saved_stdout = -1;

		// Save pid of grabber
		if (pid)
			*pid = child_pid;

		saved_stdout = dup(STDOUT_FILENO);
		dup2(pipefd[1], STDOUT_FILENO);
		close(pipefd[0]);
		close(pipefd[1]);

		exitcode = fn(context);
		if (retcode)
			*retcode = exitcode;

		fflush(stdout);
		fflush(stderr);
		dup2(saved_stdout, STDOUT_FILENO);
		close(saved_stdout);

		return BOOL_TRUE;
	}

	// Child (Output grabber)
	dup2(pipefd[0], STDIN_FILENO);
	close(pipefd[0]);
	close(pipefd[1]);
	dup2(kcontext_stdout(context), STDOUT_FILENO);
	dup2(kcontext_stderr(context), STDERR_FILENO);

	char buf[100];
	int r = -1;
	
	write(STDOUT_FILENO, "grabber:", 8);
	r = read(STDIN_FILENO, buf, 100);
	write(STDOUT_FILENO, buf, r);

	fflush(stdout);
	fflush(stderr);

	_exit(0);

	return BOOL_TRUE;
}


// === ASYNC symbol execution
// The process will be forked and sym will be executed there.
// The parent will save forked process's pid and immediately return
// control to event loop which will get forked process stdout and
// wait for process termination.
static bool_t exec_action_async(kcontext_t *context, const kaction_t *action,
	pid_t *pid)
{
	ksym_fn fn = NULL;
	int exitcode = 0;
	pid_t child_pid = -1;

	fn = ksym_function(kaction_sym(action));

	// Oh, it's amazing world of stdio!
	// Flush buffers before fork() because buffer content will be inherited
	// by child. Moreover dup2() can replace old stdout file descriptor by
	// the new one but buffer linked with stdout stream will remain the same.
	// It must be empty.
	fflush(stdout);
	fflush(stderr);

	child_pid = fork();
	if (child_pid == -1)
		return BOOL_FALSE;

	// Parent
	// Save the child pid and return control. Later event loop will wait
	// for saved pid.
	if (child_pid != 0) {
		if (pid)
			*pid = child_pid;
		return BOOL_TRUE;
	}

	// Child
	dup2(kcontext_stdin(context), STDIN_FILENO);
	dup2(kcontext_stdout(context), STDOUT_FILENO);
	dup2(kcontext_stderr(context), STDERR_FILENO);

	exitcode = fn(context);
	// We will use _exit() later so stdio streams will remain unflushed.
	// Some output data can be lost. Flush necessary streams here.
	fflush(stdout);
	fflush(stderr);
	// Use _exit() but not exit() to don't flush all the stdio streams. It
	// can be dangerous because parent can have a lot of streams inhereted
	// by child process.
	_exit(exitcode);

	return BOOL_TRUE;
}


static bool_t exec_action(kcontext_t *context, const kaction_t *action,
	pid_t *pid, int *retcode)
{
	assert(context);
	if (!context)
		return BOOL_FALSE;
	assert(action);
	if (!action)
		return BOOL_FALSE;

	if (kaction_is_sync(action))
		return exec_action_sync(context, action, pid, retcode);

	return exec_action_async(context, action, pid);
}


static bool_t exec_action_sequence(const kexec_t *exec, kcontext_t *context,
	pid_t pid, int wstatus)
{
	faux_list_node_t *iter = NULL;
	int exitstatus = WEXITSTATUS(wstatus);
	pid_t new_pid = -1; // PID of newly forked ACTION process

	assert(context);
	if (!context)
		return BOOL_FALSE;

	// There is two reasons to don't start any real actions.
	// - The ACTION sequence is already done;
	// - Passed PID (PID of completed process) is not owned by this context.
	// Returns false that indicates this PID is not mine.
	if (kcontext_done(context) || (kcontext_pid(context) != pid))
		return BOOL_FALSE;

	// Here we know that given PID is our PID
	iter = kcontext_action_iter(context); // Get saved current ACTION

	// ASYNC: Compute new value for retcode.
	// Here iter is a pointer to previous action but not new.
	// It's for async actions only. Sync actions will change global
	// retcode after the exec_action() invocation.
	if (iter) {
		const kaction_t *terminated_action = faux_list_data(iter);
		assert(terminated_action);
		if (!kaction_is_sync(terminated_action) &&
			kaction_update_retcode(terminated_action))
			kcontext_set_retcode(context, exitstatus);
	}

	// Loop is needed because some ACTIONs will be skipped due to specified
	// execution conditions. So try next actions.
	do {
		const kaction_t *action = NULL;
		bool_t is_sync = BOOL_FALSE;

		// Get next ACTION from sequence
		if (!iter) { // Is it the first ACTION within list
			faux_list_t *actions =
				kentry_actions(kpargv_command(kcontext_pargv(context)));
			assert(actions);
			iter = faux_list_head(actions);
		} else {
			iter = faux_list_next_node(iter);
		}
		kcontext_set_action_iter(context, iter);

		// Is it end of ACTION sequence?
		if (!iter) {
			kcontext_set_done(context, BOOL_TRUE);
			return BOOL_TRUE;
		}

		// Get new ACTION to execute
		action = (const kaction_t *)faux_list_data(iter);
		assert(action);

		// Check for previous retcode to find out if next command must
		// be executed or skipped.
		if (!kaction_meet_exec_conditions(action, kcontext_retcode(context)))
			continue; // Skip action, try next one

		// Check for dry-run flag and 'permanent' feature of ACTION.
		if (kexec_dry_run(exec) && !kaction_permanent(action)) {
			is_sync = BOOL_TRUE; // Simulate sync action
			exitstatus = 0; // Exit status while dry-run is always 0
		 } else { // Normal execution
			is_sync = kaction_is_sync(action);
			exec_action(context, action, &new_pid, &exitstatus);
		}

		// SYNC: Compute new value for retcode.
		// Sync actions return retcode immediatelly. Their forked
		// processes are for output handling only.
		if (is_sync && kaction_update_retcode(action))
			kcontext_set_retcode(context, exitstatus);

	} while (-1 == new_pid); // PID is not -1 when new process was forked

	// Save PID of newly created process
	kcontext_set_pid(context, new_pid);

	return BOOL_TRUE;
}


bool_t kexec_continue_command_execution(kexec_t *exec, pid_t pid, int wstatus)
{
	faux_list_node_t *iter = NULL;
	kcontext_t *context = NULL;

	assert(exec);
	if (!exec)
		return BOOL_FALSE;

	iter = kexec_contexts_iter(exec);
	while ((context = kexec_contexts_each(&iter))) {
		bool_t found = BOOL_FALSE;
		found = exec_action_sequence(exec, context, pid, wstatus);
		if (found && (pid != -1))
			break;
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
	kexec_continue_command_execution(exec, -1, 0);

	return BOOL_TRUE;
}
