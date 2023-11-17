/** @file kexec.c
 */
#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <signal.h>
#include <errno.h>

#include <faux/list.h>
#include <faux/buf.h>
#include <faux/eloop.h>
#include <klish/khelper.h>
#include <klish/kcontext.h>
#include <klish/kpath.h>
#include <klish/kexec.h>


#define PTMX_PATH "/dev/ptmx"


// Declaration of grabber. Implementation is in the grabber.c
void grabber(int fds[][2]);

struct kexec_s {
	kcontext_type_e type; // Common ACTIONs or service ACTIONs
	ksession_t *session;
	faux_list_t *contexts;
	bool_t dry_run;
	int stdin;
	int stdout;
	int stderr;
	faux_buf_t *bufin;
	faux_buf_t *bufout;
	faux_buf_t *buferr;
	kpath_t *saved_path;
	char *pts_fname; // Pseudoterminal slave file name
	int pts; // Pseudoterminal slave handler
	char *line; // Full command to execute (text)
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

// Saved path
KGET(exec, kpath_t *, saved_path);

// Line
KGET_STR(exec, line);
KSET_STR(exec, line);

// CONTEXT list
KADD_NESTED(exec, kcontext_t *, contexts);
KNESTED_LEN(exec, contexts);
KNESTED_IS_EMPTY(exec, contexts);
KNESTED_ITER(exec, contexts);
KNESTED_EACH(exec, kcontext_t *, contexts);

// Pseudoterminal
FAUX_HIDDEN KGET(exec, int, pts);
FAUX_HIDDEN KSET(exec, int, pts);
FAUX_HIDDEN KSET_STR(exec, pts_fname);
FAUX_HIDDEN KGET_STR(exec, pts_fname);


kexec_t *kexec_new(ksession_t *session, kcontext_type_e type)
{
	kexec_t *exec = NULL;

	assert(session);
	if (!session)
		return NULL;

	exec = faux_zmalloc(sizeof(*exec));
	assert(exec);
	if (!exec)
		return NULL;

	exec->type = type;
	exec->session = session;
	exec->dry_run = BOOL_FALSE;
	exec->saved_path = NULL;
	exec->line = NULL;

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

	// Pseudoterminal
	exec->pts = -1;
	exec->pts_fname = NULL;

	return exec;
}


void kexec_free(kexec_t *exec)
{
	if (!exec)
		return;

	faux_list_free(exec->contexts);

	if (exec->stdin != -1)
		close(exec->stdin);
	if (exec->stdout != -1)
		close(exec->stdout);
	if (exec->stderr != -1)
		close(exec->stderr);

	faux_buf_free(exec->bufin);
	faux_buf_free(exec->bufout);
	faux_buf_free(exec->buferr);

	faux_str_free(exec->pts_fname);
	faux_str_free(exec->line);

	kpath_free(exec->saved_path);

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


// Retcode of kexec is a 0 if all pipelined stages have retcode=0 or first
// non-null retcode else.
// Retcode valid if kexec is done. Else current
// retcode is non-valid and will not be returned at all.
bool_t kexec_retcode(const kexec_t *exec, int *status)
{
	kexec_contexts_node_t *iter = NULL;
	kcontext_t *context = NULL;

	assert(exec);
	if (!exec)
		return BOOL_FALSE;

	if (kexec_is_empty(exec))
		return BOOL_FALSE;

	if (!kexec_done(exec)) // Unfinished execution
		return BOOL_FALSE;

	if (!status) // User don't want to see retcode value
		return BOOL_TRUE;

	*status = 0;
	iter = kexec_contexts_iter(exec);
	while ((context = kexec_contexts_each(&iter))) {
		int retcode = kcontext_retcode(context);
		if (retcode != 0) {
			*status = retcode;
			break;
		}
	}

	return BOOL_TRUE;
}


bool_t kexec_path_is_changed(const kexec_t *exec)
{
	kpath_t *path = NULL;
	kcontext_t *context = NULL;

	assert(exec);
	if (!exec)
		return BOOL_FALSE;

	context = (kcontext_t *)faux_list_data(faux_list_head(exec->contexts));
	path = ksession_path(kcontext_session(context));
	if (kpath_is_equal(exec->saved_path, path))
		return BOOL_FALSE;

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


bool_t kexec_set_winsize(kexec_t *exec)
{
	size_t width = 0;
	size_t height = 0;
	struct winsize ws = {};
	int res = -1;

	if (!exec)
		return BOOL_FALSE;
	if (exec->pts < 0)
		return BOOL_FALSE;
	if (!isatty(exec->pts))
		return BOOL_FALSE;
	if (!exec->session)
		return BOOL_FALSE;

	// Set pseudo terminal window size
	width = ksession_term_width(exec->session);
	height = ksession_term_height(exec->session);
	if ((width == 0) || (height == 0))
		return BOOL_FALSE;

	ws.ws_col = (unsigned short)width;
	ws.ws_row = (unsigned short)height;
	res = ioctl(exec->pts, TIOCSWINSZ, &ws);
	if (res < 0)
		return BOOL_FALSE;

	return BOOL_TRUE;
}


static bool_t kexec_prepare(kexec_t *exec)
{
	int pipefd[2] = {};
	faux_list_node_t *iter = NULL;
	int global_stderr = -1;
	int fflags = 0;
	int r_end = -1;
	int w_end = -1;
	// Pseudoterminal related vars
	bool_t isatty_stdin = BOOL_FALSE;
	bool_t isatty_stdout = BOOL_FALSE;
	bool_t isatty_stderr = BOOL_FALSE;
	int pts = -1;
	int ptm = -1;
	char *pts_name = NULL;


	assert(exec);
	if (!exec)
		return BOOL_FALSE;
	// Nothing to prepare for empty list
	if (kexec_contexts_is_empty(exec))
		return BOOL_FALSE;

	// If user has a terminal somewhere (stdin, stdout, stderr) then prepare
	// pseudoterminal. Service actions (internal actions like PTYPE checks)
	// never get terminal
	if (exec->type == KCONTEXT_TYPE_ACTION) {
		isatty_stdin = ksession_isatty_stdin(exec->session);
		// Only if last command in pipeline is interactive then stdout
		// can be pts. Because client adds its own pager to pipeline in
		// a case of non-interactive commands
		if (kexec_interactive(exec))
			isatty_stdout = ksession_isatty_stdout(exec->session);
		isatty_stderr = ksession_isatty_stderr(exec->session);
	}
	if (isatty_stdin || isatty_stdout || isatty_stderr) {
		ptm = open(PTMX_PATH, O_RDWR, O_NOCTTY);
		if (ptm < 0)
			return BOOL_FALSE;
		// Set O_NONBLOCK flag here. Because this flag is ignored while
		// open() ptmx. I don't know why. fcntl() is working fine.
		fflags = fcntl(ptm, F_GETFL);
		fcntl(ptm, F_SETFL, fflags | O_NONBLOCK);
		grantpt(ptm);
		unlockpt(ptm);
		pts_name = ptsname(ptm);
		// In a case of pseudo-terminal the pts
		// must be reopened later in the child after setsid(). So
		// save filename of pts
		kexec_set_pts_fname(exec, pts_name);
		// Open client side (pts) of pseudo terminal. It's necessary for
		// sync action execution. Additionally open descriptor makes
		// action (from child) to don't send SIGHUP on terminal handler.
		pts = open(pts_name, O_RDWR, O_NOCTTY);
		if (pts < 0)
			return BOOL_FALSE;
		kexec_set_pts(exec, pts);
		// Set pseudo terminal window size
		kexec_set_winsize(exec);
	}

	// Create "global" stdin, stdout, stderr for the whole job execution.

	// STDIN
	if (isatty_stdin) {
		r_end = pts;
		w_end = ptm;
	} else {
		if (pipe(pipefd) < 0)
			return BOOL_FALSE;
		// Write end of 'stdin' pipe must be non-blocked
		fflags = fcntl(pipefd[1], F_GETFL);
		fcntl(pipefd[1], F_SETFL, fflags | O_NONBLOCK);
		r_end = pipefd[0];
		w_end = pipefd[1];
	}
	kcontext_set_stdin(faux_list_data(
		faux_list_head(exec->contexts)), r_end); // Read end
	kexec_set_stdin(exec, w_end); // Write end

	// STDOUT
	if (isatty_stdout) {
		r_end = ptm;
		w_end = pts;
	} else {
		if (pipe(pipefd) < 0)
			return BOOL_FALSE;
		// Read end of 'stdout' pipe must be non-blocked
		fflags = fcntl(pipefd[0], F_GETFL);
		fcntl(pipefd[0], F_SETFL, fflags | O_NONBLOCK);
		r_end = pipefd[0];
		w_end = pipefd[1];
	}
	kexec_set_stdout(exec, r_end); // Read end
	kcontext_set_stdout(
		faux_list_data(faux_list_tail(exec->contexts)), w_end); // Write end

	// STDERR
	if (isatty_stderr) {
		r_end = ptm;
		w_end = pts;
	} else {
		if (pipe(pipefd) < 0)
			return BOOL_FALSE;
		// Read end of 'stderr' pipe must be non-blocked
		fflags = fcntl(pipefd[0], F_GETFL);
		fcntl(pipefd[0], F_SETFL, fflags | O_NONBLOCK);
		r_end = pipefd[0];
		w_end = pipefd[1];
	}
	kexec_set_stderr(exec, r_end); // Read end
	// STDERR write end will be set to all list members as stderr
	global_stderr = w_end; // Write end

	// Save current path
	if (ksession_path(exec->session))
		exec->saved_path = kpath_clone(ksession_path(exec->session));

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
static bool_t exec_action_sync(const kexec_t *exec, kcontext_t *context,
	const kaction_t *action, pid_t *pid, int *retcode)
{
	ksym_fn fn = NULL;
	int exitcode = 0;
	pid_t child_pid = -1;
	int pipe_stdout[2] = {};
	int pipe_stderr[2] = {};

	// Create pipes beetween sym function and grabber
	if (pipe(pipe_stdout) < 0)
		return BOOL_FALSE;
	if (pipe(pipe_stderr) < 0) {
		close(pipe_stdout[0]);
		close(pipe_stdout[1]);
		return BOOL_FALSE;
	}

	fn = ksym_function(kaction_sym(action));

	// Prepare streams before fork
	fflush(stdout);
	fflush(stderr);

	// Fork the grabber
	child_pid = fork();
	if (child_pid == -1) {
		close(pipe_stdout[0]);
		close(pipe_stdout[1]);
		close(pipe_stderr[0]);
		close(pipe_stderr[1]);
		return BOOL_FALSE;
	}

	// Parent
	if (child_pid != 0) {
		int saved_stdout = -1;
		int saved_stderr = -1;

		// Save pid of grabber
		if (pid)
			*pid = child_pid;

		// Temporarily replace orig output streams by pipe
		// stdout
		saved_stdout = dup(STDOUT_FILENO);
		dup2(pipe_stdout[1], STDOUT_FILENO);
		close(pipe_stdout[0]);
		close(pipe_stdout[1]);
		// stderr
		saved_stderr = dup(STDERR_FILENO);
		dup2(pipe_stderr[1], STDERR_FILENO);
		close(pipe_stderr[0]);
		close(pipe_stderr[1]);

		// Execute sym function right here
		exitcode = fn(context);
		if (retcode)
			*retcode = exitcode;

		// Restore orig output streams
		// stdout
		fflush(stdout);
		close(STDOUT_FILENO);
		dup2(saved_stdout, STDOUT_FILENO);
		close(saved_stdout);
		// stderr
		fflush(stderr);
		close(STDERR_FILENO);
		dup2(saved_stderr, STDERR_FILENO);
		close(saved_stderr);

		return BOOL_TRUE;

	// Child (Output grabber)
	} else {
		int fds[][2] = {
			{pipe_stdout[0], kcontext_stdout(context)},
			{pipe_stderr[0], kcontext_stderr(context)},
			{-1, -1},
		};
		grabber(fds); // Grabber will not return
	}

	exec = exec; // Happy compiler

	return BOOL_TRUE;
}


// === ASYNC symbol execution
// The process will be forked and sym will be executed there.
// The parent will save forked process's pid and immediately return
// control to event loop which will get forked process stdout and
// wait for process termination.
static bool_t exec_action_async(const kexec_t *exec, kcontext_t *context,
	const kaction_t *action, pid_t *pid)
{
	ksym_fn fn = NULL;
	int exitcode = 0;
	pid_t child_pid = -1;
	int i = 0;
	int fdmax = 0;
	sigset_t sigs;

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

	// Unblock signals
	sigemptyset(&sigs);
	sigprocmask(SIG_SETMASK, &sigs, NULL);

	// Reopen streams if the pseudoterminal is used.
	// It's necessary to set session terminal
	if (exec->pts_fname != NULL) {
		int fd = -1;
		setsid();
		fd = open(exec->pts_fname, O_RDWR, 0);
		if (fd < 0)
			_exit(-1);
		if (isatty(kcontext_stdin(context)))
			kcontext_set_stdin(context, fd);
		if (isatty(kcontext_stdout(context)))
			kcontext_set_stdout(context, fd);
		if (isatty(kcontext_stderr(context)))
			kcontext_set_stderr(context, fd);
	}

	dup2(kcontext_stdin(context), STDIN_FILENO);
	dup2(kcontext_stdout(context), STDOUT_FILENO);
	dup2(kcontext_stderr(context), STDERR_FILENO);

	// Close all inherited fds except stdin, stdout, stderr
	fdmax = (int)sysconf(_SC_OPEN_MAX);
	for (i = (STDERR_FILENO + 1); i < fdmax; i++)
		close(i);

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


static bool_t exec_action(const kexec_t *exec, kcontext_t *context,
	const kaction_t *action, pid_t *pid, int *retcode)
{
	bool_t rc = BOOL_FALSE;

	assert(context);
	if (!context)
		return BOOL_FALSE;
	assert(action);
	if (!action)
		return BOOL_FALSE;

	if (kaction_is_sync(action))
		rc = exec_action_sync(exec, context, action, pid, retcode);
	else
		rc = exec_action_async(exec, context, action, pid);

	return rc;
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
			// Close the stdout of finished ACTION sequence to inform
			// process next in pipe about EOF. Else filter will not
			// stop at all.
			close(kcontext_stdout(context));
			kcontext_set_stdout(context, -1);
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
		if (kexec_dry_run(exec) && !kaction_is_permanent(action)) {
			is_sync = BOOL_TRUE; // Simulate sync action
			exitstatus = 0; // Exit status while dry-run is always 0
		 } else { // Normal execution
			is_sync = kaction_is_sync(action);
			exec_action(exec, context, action, &new_pid, &exitstatus);
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
	kcontext_t *context = NULL;
	const kpargv_t *pargv = NULL;
	const kentry_t *entry = NULL;
	bool_t restore = BOOL_FALSE;

	assert(exec);
	if (!exec)
		return BOOL_FALSE;

	// Firsly prepare kexec object for execution. The file streams must
	// be created for stdin, stdout, stderr of processes.
	if (!kexec_prepare(exec))
		return BOOL_FALSE;

	// Pre-change VIEW if command has "restore" flag. Only first command in
	// line (if many commands are piped) matters. Filters can't change the
	// VIEW.
	context = (kcontext_t *)faux_list_data(faux_list_head(exec->contexts));
	pargv = kcontext_pargv(context);
	entry = kpargv_command(pargv);
	if (entry)
		restore = kentry_restore(entry);
	if (restore) {
		size_t level = kpargv_level(pargv);
		kpath_t *path = ksession_path(kcontext_session(context));
		while(kpath_len(path) > (level + 1))
			kpath_pop(path);
	}

	// Here no ACTIONs are executing, so pass -1 as pid of terminated
	// ACTION's process.
	kexec_continue_command_execution(exec, -1, 0);

	return BOOL_TRUE;
}


// If some kexec's kentry has tty as "out" then consider kexec as interactive
bool_t kexec_interactive(const kexec_t *exec)
{
	faux_list_node_t *iter = NULL;
	kcontext_t *context = NULL;

	assert(exec);
	if (!exec)
		return BOOL_FALSE;

	iter = kexec_contexts_iter(exec);
	while ((context = kexec_contexts_each(&iter))) {
		const kentry_t *entry = kcontext_command(context);
		if (!entry)
			return BOOL_FALSE;
		if (kentry_out(entry) == KACTION_IO_TTY)
			return BOOL_TRUE;
	}

	return BOOL_FALSE;
}


// If some kexec's kentry has tty as "in" then consider kexec as need_stdin.
// The first kentry with "in=true" also does kexec need_stdin
bool_t kexec_need_stdin(const kexec_t *exec)
{
	faux_list_node_t *iter = NULL;
	size_t num = 0;
	kcontext_t *context = NULL;

	assert(exec);
	if (!exec)
		return BOOL_FALSE;

	iter = kexec_contexts_iter(exec);
	while ((context = kexec_contexts_each(&iter))) {
		const kentry_t *entry = kcontext_command(context);
		if (!entry)
			return BOOL_FALSE;
		// Check first command within pipeline
		if (num == 0) {
			if (kentry_in(entry) == KACTION_IO_TRUE)
				return BOOL_TRUE;
		}
		num++;
		if (kentry_in(entry) == KACTION_IO_TTY)
			return BOOL_TRUE;
	}

	return BOOL_FALSE;
}


const kaction_t *kexec_current_action(const kexec_t *exec)
{
	kcontext_t *context = NULL;

	assert(exec);
	if (!exec)
		return NULL;

	context = (kcontext_t *)faux_list_data(faux_list_head(exec->contexts));
	if (!context)
		return NULL;

	return kcontext_action(context);
}
