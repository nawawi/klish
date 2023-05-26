/** @file ksession_parse.c
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <syslog.h>

#include <faux/eloop.h>
#include <faux/buf.h>
#include <faux/list.h>
#include <faux/argv.h>
#include <faux/error.h>
#include <klish/khelper.h>
#include <klish/kscheme.h>
#include <klish/kpath.h>
#include <klish/kpargv.h>
#include <klish/kexec.h>
#include <klish/ksession.h>
#include <klish/ksession_parse.h>


static bool_t ksession_validate_arg(ksession_t *session, kpargv_t *pargv)
{
	char *out = NULL;
	int retcode = -1;
	const kentry_t *ptype_entry = NULL;
	kparg_t *candidate = NULL;

	assert(session);
	if (!session)
		return BOOL_FALSE;
	assert(pargv);
	if (!pargv)
		return BOOL_FALSE;
	candidate = kpargv_candidate_parg(pargv);
	if (!candidate)
		return BOOL_FALSE;
	ptype_entry = kentry_nested_by_purpose(kparg_entry(candidate),
		KENTRY_PURPOSE_PTYPE);
	if (!ptype_entry)
		return BOOL_FALSE;

	if (!ksession_exec_locally(session, ptype_entry, pargv,
		&retcode, &out)) {
		return BOOL_FALSE;
	}

	if (retcode != 0)
		return BOOL_FALSE;

	if (!faux_str_is_empty(out))
		kparg_set_value(candidate, out);

	return BOOL_TRUE;
}


static kpargv_status_e ksession_parse_arg(ksession_t *session,
	const kentry_t *current_entry, faux_argv_node_t **argv_iter,
	kpargv_t *pargv, bool_t entry_is_command, bool_t is_filter)
{
	const kentry_t *entry = current_entry;
	kentry_mode_e mode = KENTRY_MODE_NONE;
	kpargv_status_e retcode = KPARSE_INPROGRESS; // For ENTRY itself
	kpargv_status_e rc = KPARSE_NOTFOUND; // For nested ENTRYs
	faux_argv_node_t *saved_argv_iter = NULL;
	kpargv_purpose_e purpose = KPURPOSE_NONE;

//fprintf(stderr, "PARSE: name=%s, ref=%s, arg=%s, pargs=%d\n",
//kentry_name(entry), kentry_ref_str(entry), faux_argv_current(*argv_iter),
//kpargv_pargs_len(pargv));

	assert(current_entry);
	if (!current_entry)
		return KPARSE_ERROR;
	assert(argv_iter);
	if (!argv_iter)
		return KPARSE_ERROR;
	assert(pargv);
	if (!pargv)
		return KPARSE_ERROR;

	purpose = kpargv_purpose(pargv); // Purpose of parsing

	// If we know the entry is a command then don't validate it. This
	// behaviour is usefull for special purpose entries like PTYPEs, CONDs,
	// etc. These entries are the starting point for parsing their args.
	// We don't need to parse command itself. Command is predefined.
	if (entry_is_command) {
		kparg_t *parg = NULL;

		// Command is an ENTRY with ACTIONs
		if (kentry_actions_len(entry) <= 0)
			return KPARSE_ILLEGAL;
		parg = kparg_new(entry, NULL);
		kpargv_add_pargs(pargv, parg);
		kpargv_set_command(pargv, entry);
		retcode = KPARSE_INPROGRESS;

	// Is entry candidate to resolve current arg?
	// Container can't be a candidate.
	} else if (!kentry_container(entry)) {
		const char *current_arg = NULL;
		kparg_t *parg = NULL;

		// When purpose is COMPLETION or HELP then fill completion list.
		// Additionally if it's last continuable argument then lie to
		// engine: make all last arguments NOTFOUND. It's necessary to walk
		// through all variants to gather all completions.
		// is_filter: When it's a filter then all non-first entries can be
		// filters or non-filters
		if (((KPURPOSE_COMPLETION == purpose) ||
			(KPURPOSE_HELP == purpose)) &&
			((is_filter == kentry_filter(entry)) ||
			(is_filter && kpargv_pargs_len(pargv)))) {
			if (!*argv_iter) {
				// That's time to add entry to completions list.
				if (!kpargv_continuable(pargv))
					kpargv_add_completions(pargv, entry);
				return KPARSE_INCOMPLETED;
			} else {
				// Add entry to completions if it's last incompleted arg.
				if (faux_argv_is_last(*argv_iter) &&
					kpargv_continuable(pargv)) {
					kpargv_add_completions(pargv, entry);
					return KPARSE_NOTFOUND;
				}
			}
		}

		// If all arguments are resolved already then return INCOMPLETED
		if (!*argv_iter)
			return KPARSE_INCOMPLETED;

		// Validate argument
		current_arg = faux_argv_current(*argv_iter);
		parg = kparg_new(entry, current_arg);
		kpargv_set_candidate_parg(pargv, parg);
		if (ksession_validate_arg(session, pargv)) {
			kpargv_accept_candidate_parg(pargv);
			// Command is an ENTRY with ACTIONs or NAVigation
			if (kentry_actions_len(entry) > 0)
				kpargv_set_command(pargv, entry);
			faux_argv_each(argv_iter); // Next argument
			retcode = KPARSE_INPROGRESS;
		} else {
			// It's not a container and is not validated so
			// no chance to find anything here.
			kpargv_decline_candidate_parg(pargv);
			kparg_free(parg);
			return KPARSE_NOTFOUND;
		}
	}

	// ENTRY has no nested ENTRYs so return
	if (kentry_entrys_is_empty(entry))
		return retcode;

	// Walk through the nested entries:
	saved_argv_iter = *argv_iter;

	// EMPTY mode
	mode = kentry_mode(entry);
	if (KENTRY_MODE_EMPTY == mode)
		return retcode;

	// Following code (SWITCH or SEQUENCE cases) sometimes don's set rc.
	// It happens when entry has nested entries but purposes of all entries
	// are not COMMON so they will be ignored. So return code of function
	// will be the code of ENTRY itself processing.
	rc = retcode;

	// SWITCH mode
	// Entries within SWITCH can't has 'min'/'max' else than 1.
	// So these attributes will be ignored. Note SWITCH itself can have
	// 'min'/'max'.
	if (KENTRY_MODE_SWITCH == mode) {
		kentry_entrys_node_t *iter = kentry_entrys_iter(entry);
		const kentry_t *nested = NULL;

		while ((nested = kentry_entrys_each(&iter))) {
//printf("SWITCH arg: %s, entry %s\n", *argv_iter ? faux_argv_current(*argv_iter) : "<empty>", kentry_name(nested));
			// Ignore entries with non-COMMON purpose.
			if (kentry_purpose(nested) != KENTRY_PURPOSE_COMMON)
				continue;
			rc = ksession_parse_arg(session, nested, argv_iter,
				pargv, BOOL_FALSE, is_filter);
//printf("%s\n", kpargv_status_decode(rc));
			// If some arguments was consumed then we will not check
			// next SWITCH's entries in any case.
			if (saved_argv_iter != *argv_iter)
				break;
			// Try next entries if current status is NOTFOUND.
			// The INCOMPLETED status is for completion list. In this
			// case all next statuses will be INCOMPLETED too.
			if ((rc != KPARSE_NOTFOUND) && (rc != KPARSE_INCOMPLETED))
				break;
		}

	// SEQUENCE mode
	} else if (KENTRY_MODE_SEQUENCE == mode) {
		kentry_entrys_node_t *iter = kentry_entrys_iter(entry);
		kentry_entrys_node_t *saved_iter = iter;
		const kentry_t *nested = NULL;

		while ((nested = kentry_entrys_each(&iter))) {
			kpargv_status_e nrc = KPARSE_NOTFOUND;
			size_t num = 0;
			size_t min = kentry_min(nested);

			// Ignore entries with non-COMMON purpose.
			if (kentry_purpose(nested) != KENTRY_PURPOSE_COMMON)
				continue;
			// Filter out double parsing for optional entries.
			if (kpargv_entry_exists(pargv, nested))
				continue;
fprintf(stderr, "SEQ arg: %s, entry %s\n", *argv_iter ? faux_argv_current(*argv_iter) : "<empty>", kentry_name(nested));
			// Try to match argument and current entry
			// (from 'min' to 'max' times)
			for (num = 0; num < kentry_max(nested); num++) {
				faux_argv_node_t *iter_before_parse = *argv_iter;
				nrc = ksession_parse_arg(session, nested,
					argv_iter, pargv, BOOL_FALSE, is_filter);
fprintf(stderr, "%s: %s\n", kentry_name(nested), kpargv_status_decode(nrc));
				if (nrc != KPARSE_INPROGRESS)
					break;
				if (iter_before_parse == *argv_iter)
					break;
			}
			// All errors will break the loop
			if ((KPARSE_ERROR == nrc) ||
				(KPARSE_ILLEGAL == nrc) ||
				(KPARSE_NONE == nrc)) {
				rc = nrc;
				break;
			}
			// Not found necessary number of mandatory instances
			if (num < min) {
				if (KPARSE_INPROGRESS == nrc)
					rc = KPARSE_NOTFOUND;
				else
					rc = nrc; // NOTFOUND or INCOMPLETED
				break;
			}
			// It's not an error if optional parameter is absend
			rc = KPARSE_INPROGRESS;
			// Mandatory or ordered parameter
			if ((min > 0) || kentry_order(nested))
				saved_iter = iter;

			// If optional entry is found then go back to nearest
			// non-optional (or ordered) entry to try to find
			// another optional entries.
fprintf(stderr, "%s: min=%d, num=%d\n", kentry_name(nested), min, num);
			if ((0 == min) && (num > 0))
				iter = saved_iter;
		}
	}

	// If nested result is NOTFOUND but argument was consumed
	// within nested entries or by entry itself then whole sequence
	// is ILLEGAL.
	if ((KPARSE_NOTFOUND == rc) &&
		((saved_argv_iter != *argv_iter) || !kentry_container(entry)))
		rc = KPARSE_ILLEGAL;

fprintf(stderr, "Return %s: %s\n", kentry_name(entry), kpargv_status_decode(rc));
	return rc;
}


kpargv_t *ksession_parse_line(ksession_t *session, const faux_argv_t *argv,
	kpargv_purpose_e purpose, bool_t is_filter)
{
	faux_argv_node_t *argv_iter = NULL;
	kpargv_t *pargv = NULL;
	kpargv_status_e pstatus = KPARSE_NONE;
	kpath_levels_node_t *levels_iterr = NULL;
	klevel_t *level = NULL;
	size_t level_found = 0; // Level where command was found
	kpath_t *path = NULL;

	assert(session);
	if (!session)
		return NULL;
	assert(argv);
	if (!argv)
		return NULL;

	argv_iter = faux_argv_iter(argv);

	// Initialize kpargv_t
	pargv = kpargv_new();
	assert(pargv);
	kpargv_set_continuable(pargv, faux_argv_is_continuable(argv));
	kpargv_set_purpose(pargv, purpose);

	// Iterate levels of path from higher to lower. Note the reversed
	// iterator will be used.
	path = ksession_path(session);
	levels_iterr = kpath_iterr(path);
	level_found = kpath_len(path) - 1; // Levels begin with '0'
	while ((level = kpath_eachr(&levels_iterr))) {
		const kentry_t *current_entry = klevel_entry(level);
		// Ignore entries with non-COMMON purpose. These entries are for
		// special processing and will be ignored here.
		if (kentry_purpose(current_entry) != KENTRY_PURPOSE_COMMON)
			continue;
		// Parsing
		pstatus = ksession_parse_arg(session, current_entry, &argv_iter,
			pargv, BOOL_FALSE, is_filter);
		if (pstatus != KPARSE_NOTFOUND)
			break;
		// NOTFOUND but some args were parsed.
		// When it's completion for first argument (that can be continued)
		// len == 0 and engine will search for completions on higher
		// levels of path.
		if (kpargv_pargs_len(pargv) > 0)
			break;
		level_found--;
	}
	// Save last argument
	if (argv_iter)
		kpargv_set_last_arg(pargv, faux_argv_current(argv_iter));
	// It's a higher level of parsing, so some statuses can have different
	// meanings
	if (KPARSE_NONE == pstatus)
		pstatus = KPARSE_ERROR; // Strange case
	else if (KPARSE_INPROGRESS == pstatus) {
		if (NULL == argv_iter) // All args are parsed
			pstatus = KPARSE_OK;
		else
			pstatus = KPARSE_ILLEGAL; // Additional not parsable args
	} else if (KPARSE_NOTFOUND == pstatus)
		pstatus = KPARSE_ILLEGAL; // Unknown command
	// If no ACTIONs were found i.e. command was not found
	if ((KPARSE_OK == pstatus) && !kpargv_command(pargv))
		pstatus = KPARSE_NOACTION;

	kpargv_set_status(pargv, pstatus);
	kpargv_set_level(pargv, level_found);

	return pargv;
}


// Delimeter of commands is '|' (pipe)
faux_list_t *ksession_split_pipes(const char *raw_line, faux_error_t *error)
{
	faux_list_t *list = NULL;
	faux_argv_t *argv = NULL;
	faux_argv_node_t *argv_iter = NULL;
	faux_argv_t *cur_argv = NULL; // Current argv
	const char *delimeter = "|";
	const char *arg = NULL;

	assert(raw_line);
	if (!raw_line)
		return NULL;

	// Split raw line to arguments
	argv = faux_argv_new();
	assert(argv);
	if (!argv)
		return NULL;
	if (faux_argv_parse(argv, raw_line) < 0) {
		faux_argv_free(argv);
		return NULL;
	}

	list = faux_list_new(FAUX_LIST_UNSORTED, FAUX_LIST_NONUNIQUE,
		NULL, NULL, (void (*)(void *))faux_argv_free);
	assert(list);
	if (!list) {
		faux_argv_free(argv);
		return NULL;
	}

	argv_iter = faux_argv_iter(argv);
	cur_argv = faux_argv_new();
	assert(cur_argv);
	while ((arg = faux_argv_each(&argv_iter))) {
		if (strcmp(arg, delimeter) == 0) {
			// End of current line (from "|" to "|")
			// '|' in a first position is an error
			if (faux_argv_len(cur_argv) == 0) {
				faux_argv_free(argv);
				faux_list_free(list);
				faux_error_sprintf(error, "The pipe '|' can't "
					"be at the first position");
				return NULL;
			}
			// Add argv to argv's list
			faux_list_add(list, cur_argv);
			cur_argv = faux_argv_new();
			assert(cur_argv);
		} else {
			faux_argv_add(cur_argv, arg);
		}
	}

	// Continuable flag is usefull for last argv
	faux_argv_set_continuable(cur_argv, faux_argv_is_continuable(argv));
	// Empty cur_argv is not an error. It's usefull for completion and help.
	// But empty cur_argv and continuable is abnormal.
	if ((faux_argv_len(cur_argv) == 0) &&
		faux_argv_is_continuable(cur_argv)) {
		faux_argv_free(argv);
		faux_list_free(list);
		faux_error_sprintf(error, "The pipe '|' can't "
			"be the last argument");
		return NULL;
	}
	faux_list_add(list, cur_argv);

	faux_argv_free(argv);

	return list;
}


// is_piped means full command contains more than one piped components
static bool_t ksession_check_line(const kpargv_t *pargv, faux_error_t *error,
	bool_t is_first, bool_t is_piped)
{
	kpargv_purpose_e purpose = KPURPOSE_EXEC;
	const kentry_t *cmd = NULL;

	if (!pargv)
		return BOOL_FALSE;

	purpose = kpargv_purpose(pargv);
	cmd = kpargv_command(pargv);

	// For execution pargv must be fully correct but for completion
	// it's not a case
	if ((KPURPOSE_EXEC == purpose) && (kpargv_status(pargv) != KPARSE_OK)) {
		faux_error_sprintf(error, "%s", kpargv_status_str(pargv));
		return BOOL_FALSE;
	}

	// Can't check following conditions without cmd
	if (!cmd)
		return BOOL_TRUE;

	// First component
	if (is_first) {

		// First component can't be filter
		if (kentry_filter(cmd)) {
			faux_error_sprintf(error, "The filter \"%s\" "
				"can't be used without previous pipeline",
				kentry_name(cmd));
			return BOOL_FALSE;
		}

		// Interactive command can't have filters
		if (kentry_interactive(cmd) && is_piped) {
			faux_error_sprintf(error, "The interactive command \"%s\" "
				"can't have filters",
				kentry_name(cmd));
			return BOOL_FALSE;
		}

	// Components after pipe "|"
	} else {

		// Only the first component can be non-filter
		if (!kentry_filter(cmd)) {
			faux_error_sprintf(error, "The non-filter command \"%s\" "
				"can't be destination of pipe",
				kentry_name(cmd));
			return BOOL_FALSE;
		}

		// Only the first component can have 'restore=true' attribute
		if (kentry_restore(cmd)) {
			faux_error_sprintf(error, "The command \"%s\" "
				"can't be destination of pipe",
				kentry_name(cmd));
			return BOOL_FALSE;
		}

		// Only the first component can have 'interactive=true' attribute
		if (kentry_interactive(cmd)) {
			faux_error_sprintf(error, "The filter \"%s\" "
				"can't be interactive",
				kentry_name(cmd));
			return BOOL_FALSE;
		}

	}

	return BOOL_TRUE;
}


// All components except last one must be legal for execution but last
// component must be parsed for completion.
// Completion is a "back-end" operation so it doesn't need detailed error
// reporting.
kpargv_t *ksession_parse_for_completion(ksession_t *session,
	const char *raw_line)
{
	faux_list_t *split = NULL;
	faux_list_node_t *iter = NULL;
	kpargv_t *pargv = NULL;
	bool_t is_piped = BOOL_FALSE;

	assert(session);
	if (!session)
		return NULL;
	assert(raw_line);
	if (!raw_line)
		return NULL;

	// Split raw line (with '|') to components
	split = ksession_split_pipes(raw_line, NULL);
	if (!split || (faux_list_len(split) < 1)) {
		faux_list_free(split);
		return NULL;
	}
	is_piped = (faux_list_len(split) > 1);

	iter = faux_list_head(split);
	while (iter) {
		faux_argv_t *argv = (faux_argv_t *)faux_list_data(iter);
		bool_t is_last = (iter == faux_list_tail(split));
		bool_t is_first = (iter == faux_list_head(split));
		kpargv_purpose_e purpose = is_last ? KPURPOSE_COMPLETION : KPURPOSE_EXEC;

		pargv = ksession_parse_line(session, argv, purpose, !is_first);
		if (!ksession_check_line(pargv, NULL, is_first, is_piped)) {
			kpargv_free(pargv);
			pargv = NULL;
			break;
		}
		if (!is_last)
			kpargv_free(pargv);
		iter = faux_list_next_node(iter);
	}

	faux_list_free(split);

	return pargv;
}


kexec_t *ksession_parse_for_exec(ksession_t *session, const char *raw_line,
	faux_error_t *error)
{
	faux_list_t *split = NULL;
	faux_list_node_t *iter = NULL;
	kpargv_t *pargv = NULL;
	kexec_t *exec = NULL;
	bool_t is_piped = BOOL_FALSE;

	assert(session);
	if (!session)
		return NULL;
	assert(raw_line);
	if (!raw_line)
		return NULL;

	// Split raw line (with '|') to components
	split = ksession_split_pipes(raw_line, error);
	if (!split || (faux_list_len(split) < 1)) {
		faux_list_free(split);
		return NULL;
	}
	is_piped = (faux_list_len(split) > 1);

	// Create exec list
	exec = kexec_new();
	assert(exec);
	if (!exec) {
		faux_list_free(split);
		return NULL;
	}

	iter = faux_list_head(split);
	while (iter) {
		faux_argv_t *argv = (faux_argv_t *)faux_list_data(iter);
		kcontext_t *context = NULL;
		bool_t is_first = (iter == faux_list_head(split));

		pargv = ksession_parse_line(session, argv, KPURPOSE_EXEC, !is_first);
		// All components must be ready for execution
		if (!ksession_check_line(pargv, error, is_first, is_piped)) {
			kpargv_free(pargv);
			kexec_free(exec);
			faux_list_free(split);
			return NULL;
		}

		// Fill the kexec_t
		context = kcontext_new(KCONTEXT_TYPE_ACTION);
		assert(context);
		kcontext_set_scheme(context, ksession_scheme(session));
		kcontext_set_pargv(context, pargv);
		// Context for ACTION execution contains session
		kcontext_set_session(context, session);
		kexec_add_contexts(exec, context);

		// Next component
		iter = faux_list_next_node(iter);
	}

	faux_list_free(split);

	return exec;
}


kexec_t *ksession_parse_for_local_exec(ksession_t *session,
	const kentry_t *entry, const kpargv_t *parent_pargv)
{
	faux_argv_node_t *argv_iter = NULL;
	kpargv_t *pargv = NULL;
	kexec_t *exec = NULL;
	faux_argv_t *argv = NULL;
	kcontext_t *context = NULL;
	kpargv_status_e pstatus = KPARSE_NONE;
	const char *line = NULL; // TODO: Must be 'line' field of ENTRY

	assert(entry);
	if (!entry)
		return NULL;

	exec = kexec_new();
	assert(exec);

	argv = faux_argv_new();
	assert(argv);
	faux_argv_parse(argv, line);
	argv_iter = faux_argv_iter(argv);

	pargv = kpargv_new();
	assert(pargv);
	kpargv_set_continuable(pargv, faux_argv_is_continuable(argv));
	kpargv_set_purpose(pargv, KPURPOSE_EXEC);

	pstatus = ksession_parse_arg(session, entry, &argv_iter, pargv,
		BOOL_TRUE, BOOL_FALSE);
	// Parsing problems
	if ((pstatus != KPARSE_INPROGRESS) || (argv_iter != NULL)) {
		kexec_free(exec);
		faux_argv_free(argv);
		kpargv_free(pargv);
		return NULL;
	}

	context = kcontext_new(KCONTEXT_TYPE_SERVICE_ACTION);
	assert(context);
	kcontext_set_scheme(context, ksession_scheme(session));
	kcontext_set_pargv(context, pargv);
	kcontext_set_parent_pargv(context, parent_pargv);
	kcontext_set_session(context, session);
	kexec_add_contexts(exec, context);

	faux_argv_free(argv);

	return exec;
}


static bool_t stop_loop_ev(faux_eloop_t *eloop, faux_eloop_type_e type,
	void *associated_data, void *user_data)
{
	ksession_t *session = (ksession_t *)user_data;

	if (!session)
		return BOOL_FALSE;

	ksession_set_done(session, BOOL_TRUE); // Stop the whole session

	// Happy compiler
	eloop = eloop;
	type = type;
	associated_data = associated_data;

	return BOOL_FALSE; // Stop Event Loop
}


static bool_t get_stdout(kexec_t *exec)
{
	ssize_t r = -1;
	faux_buf_t *faux_buf = NULL;
	void *linear_buf = NULL;
	int fd = -1;

	if (!exec)
		return BOOL_FALSE;

	fd = kexec_stdout(exec);
	assert(fd != -1);
	faux_buf = kexec_bufout(exec);
	assert(faux_buf);

	do {
		ssize_t really_readed = 0;
		ssize_t linear_len =
			faux_buf_dwrite_lock_easy(faux_buf, &linear_buf);
		// Non-blocked read. The fd became non-blocked while
		// kexec_prepare().
		r = read(fd, linear_buf, linear_len);
		if (r > 0)
			really_readed = r;
		faux_buf_dwrite_unlock_easy(faux_buf, really_readed);
	} while (r > 0);

	return BOOL_TRUE;
}


static bool_t action_terminated_ev(faux_eloop_t *eloop, faux_eloop_type_e type,
	void *associated_data, void *user_data)
{
	int wstatus = 0;
	pid_t child_pid = -1;
	kexec_t *exec = (kexec_t *)user_data;

	if (!exec)
		return BOOL_FALSE;

	// Wait for any child process. Doesn't block.
	while ((child_pid = waitpid(-1, &wstatus, WNOHANG)) > 0)
		kexec_continue_command_execution(exec, child_pid, wstatus);

	// Check if kexec is done now
	if (kexec_done(exec)) {
		// May be buffer still contains data
		get_stdout(exec);
		return BOOL_FALSE; // To break a loop
	}

	// Happy compiler
	eloop = eloop;
	type = type;
	associated_data = associated_data;

	return BOOL_TRUE;
}


static bool_t action_stdout_ev(faux_eloop_t *eloop, faux_eloop_type_e type,
	void *associated_data, void *user_data)
{
	kexec_t *exec = (kexec_t *)user_data;

	// Happy compiler
	eloop = eloop;
	type = type;
	associated_data = associated_data;

	return get_stdout(exec);
}


bool_t ksession_exec_locally(ksession_t *session, const kentry_t *entry,
	kpargv_t *parent_pargv, int *retcode, char **out)
{
	kexec_t *exec = NULL;
	faux_eloop_t *eloop = NULL;
	faux_buf_t *buf = NULL;
	char *cstr = NULL;
	ssize_t len = 0;

	assert(entry);
	if (!entry)
		return BOOL_FALSE;

	// Parsing
	exec = ksession_parse_for_local_exec(session, entry, parent_pargv);
	if (!exec)
		return BOOL_FALSE;

	// Session status can be changed while parsing because it can execute
	// nested ksession_exec_locally() to check for PTYPEs, CONDitions etc.
	// So check for 'done' flag to propagate it.
// NOTE: Don't interrupt single kexec_t. Let's it to complete.
//	if (ksession_done(session)) {
//		kexec_free(exec);
//		return BOOL_FALSE; // Because action is not completed
//	}

	// Execute kexec and then wait for completion using local Eloop
	if (!kexec_exec(exec)) {
		kexec_free(exec);
		return BOOL_FALSE; // Something went wrong
	}
	// If kexec contains only non-exec (for example dry-run) ACTIONs then
	// we don't need event loop and can return here.
	if (kexec_retcode(exec, retcode)) {
		kexec_free(exec);
		return BOOL_TRUE;
	}

	// Local service loop
	eloop = faux_eloop_new(NULL);
	faux_eloop_add_signal(eloop, SIGINT, stop_loop_ev, session);
	faux_eloop_add_signal(eloop, SIGTERM, stop_loop_ev, session);
	faux_eloop_add_signal(eloop, SIGQUIT, stop_loop_ev, session);
	faux_eloop_add_signal(eloop, SIGCHLD, action_terminated_ev, exec);
	faux_eloop_add_fd(eloop, kexec_stdout(exec), POLLIN,
		action_stdout_ev, exec);
	faux_eloop_loop(eloop);
	faux_eloop_free(eloop);

	kexec_retcode(exec, retcode);

	if (!out) {
		kexec_free(exec);
		return BOOL_TRUE;
	}
	buf = kexec_bufout(exec);
	if ((len = faux_buf_len(buf)) <= 0) {
		kexec_free(exec);
		return BOOL_TRUE;
	}
	cstr = faux_malloc(len + 1);
	faux_buf_read(buf, cstr, len);
	cstr[len] = '\0';
	*out = cstr;

	kexec_free(exec);

	return BOOL_TRUE;
}
