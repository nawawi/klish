#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <syslog.h>

#include <faux/faux.h>
#include <faux/str.h>
#include <faux/eloop.h>
#include <klish/ktp.h>
#include <klish/ktp_session.h>
#include <tinyrl/tinyrl.h>

#include "private.h"


// Context for main loop
typedef struct ctx_s {
	ktp_session_t *ktp;
	tinyrl_t *tinyrl;
	struct options *opts;
	char *hotkeys[VT100_HOTKEY_MAP_LEN];
	tri_t pager_working;
	FILE *pager_pipe;
} ctx_t;


bool_t auth_ack_cb(ktp_session_t *ktp, const faux_msg_t *msg, void *udata);
bool_t cmd_ack_cb(ktp_session_t *ktp, const faux_msg_t *msg, void *udata);
bool_t cmd_incompleted_ack_cb(ktp_session_t *ktp, const faux_msg_t *msg, void *udata);
bool_t completion_ack_cb(ktp_session_t *ktp, const faux_msg_t *msg, void *udata);
bool_t help_ack_cb(ktp_session_t *ktp, const faux_msg_t *msg, void *udata);

static bool_t stdin_cb(faux_eloop_t *eloop, faux_eloop_type_e type,
	void *associated_data, void *user_data);
static bool_t sigwinch_cb(faux_eloop_t *eloop, faux_eloop_type_e type,
	void *associated_data, void *user_data);

static bool_t ktp_sync_auth(ktp_session_t *ktp, int *retcode,
	faux_error_t *error);
static void reset_hotkey_table(ctx_t *ctx);
static bool_t interactive_stdout_cb(ktp_session_t *ktp, const char *line, size_t len,
	void *user_data);
static bool_t send_winch_notification(ctx_t *ctx);

// Keys
static bool_t tinyrl_key_enter(tinyrl_t *tinyrl, unsigned char key);
static bool_t tinyrl_key_tab(tinyrl_t *tinyrl, unsigned char key);
static bool_t tinyrl_key_help(tinyrl_t *tinyrl, unsigned char key);
static bool_t tinyrl_key_hotkey(tinyrl_t *tinyrl, unsigned char key);


int klish_interactive_shell(ktp_session_t *ktp, struct options *opts)
{
	ctx_t ctx = {};
	faux_eloop_t *eloop = NULL;
	tinyrl_t *tinyrl = NULL;
	int stdin_flags = 0;
	char *hist_path = NULL;
	int auth_rc = -1;

	assert(ktp);
	if (!ktp)
		return -1;

	// Set stdin to O_NONBLOCK mode
	stdin_flags = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, stdin_flags | O_NONBLOCK);

	hist_path = faux_expand_tilde("~/.klish_history");
	tinyrl = tinyrl_new(stdin, stdout, hist_path, 100);
	faux_str_free(hist_path);
	tinyrl_set_prompt(tinyrl, "$ ");
	tinyrl_set_udata(tinyrl, &ctx);
	tinyrl_set_hotkey_fn(tinyrl, tinyrl_key_hotkey);
	tinyrl_bind_key(tinyrl, '\n', tinyrl_key_enter);
	tinyrl_bind_key(tinyrl, '\r', tinyrl_key_enter);
	tinyrl_bind_key(tinyrl, '\t', tinyrl_key_tab);
	tinyrl_bind_key(tinyrl, '?', tinyrl_key_help);

	ctx.ktp = ktp;
	ctx.tinyrl = tinyrl;
	ctx.opts = opts;
	faux_bzero(ctx.hotkeys, sizeof(ctx.hotkeys));
	ctx.pager_working = TRI_UNDEFINED;
	ctx.pager_pipe = NULL;

	// Now AUTH command is used only for starting hand-shake and getting
	// prompt from the server. Generally it must be necessary for
	// non-interactive session too but for now is not implemented.
	ktp_session_set_cb(ktp, KTP_SESSION_CB_AUTH_ACK, auth_ack_cb, &ctx);
	if (!ktp_sync_auth(ktp, &auth_rc, ktp_session_error(ktp)))
		goto cleanup;
	if (auth_rc < 0)
		goto cleanup;

	// Replace common stdout callback by interactive-specific one.
	// Place it after auth to make auth use standard stdout callback.
	ktp_session_set_cb(ktp, KTP_SESSION_CB_STDOUT, interactive_stdout_cb, &ctx);

	// Don't stop interactive loop on each answer
	ktp_session_set_stop_on_answer(ktp, BOOL_FALSE);

	ktp_session_set_cb(ktp, KTP_SESSION_CB_CMD_ACK, cmd_ack_cb, &ctx);
	ktp_session_set_cb(ktp, KTP_SESSION_CB_CMD_ACK_INCOMPLETED,
		cmd_incompleted_ack_cb, &ctx);
	ktp_session_set_cb(ktp, KTP_SESSION_CB_COMPLETION_ACK,
		completion_ack_cb, &ctx);
	ktp_session_set_cb(ktp, KTP_SESSION_CB_HELP_ACK, help_ack_cb, &ctx);

	tinyrl_redisplay(tinyrl);

	eloop = ktp_session_eloop(ktp);

	// Notify server about terminal window size change
	faux_eloop_add_signal(eloop, SIGWINCH, sigwinch_cb, &ctx);

	faux_eloop_add_fd(eloop, STDIN_FILENO, POLLIN, stdin_cb, &ctx);

	send_winch_notification(&ctx);

	faux_eloop_loop(eloop);

cleanup:
	// Cleanup
	reset_hotkey_table(&ctx);
	if (tinyrl_busy(tinyrl))
		faux_error_free(ktp_session_error(ktp));
	tinyrl_free(tinyrl);

	// Restore stdin mode
	fcntl(STDIN_FILENO, F_SETFL, stdin_flags);

	return 0;
}


static bool_t process_prompt_param(tinyrl_t *tinyrl, const faux_msg_t *msg)
{
	char *prompt = NULL;

	if (!tinyrl)
		return BOOL_FALSE;
	if (!msg)
		return BOOL_FALSE;

	prompt = faux_msg_get_str_param_by_type(msg, KTP_PARAM_PROMPT);
	if (prompt) {
		tinyrl_set_prompt(tinyrl, prompt);
		faux_str_free(prompt);
	}

	return BOOL_TRUE;
}


static void reset_hotkey_table(ctx_t *ctx)
{
	size_t i = 0;

	assert(ctx);

	for (i = 0; i < VT100_HOTKEY_MAP_LEN; i++)
		faux_str_free(ctx->hotkeys[i]);
	faux_bzero(ctx->hotkeys, sizeof(ctx->hotkeys));
}


static bool_t process_hotkey_param(ctx_t *ctx, const faux_msg_t *msg)
{
	faux_list_node_t *iter = NULL;
	uint32_t param_len = 0;
	char *param_data = NULL;
	uint16_t param_type = 0;

	if (!ctx)
		return BOOL_FALSE;
	if (!msg)
		return BOOL_FALSE;

	if (!faux_msg_get_param_by_type(msg, KTP_PARAM_HOTKEY,
		(void **)&param_data, &param_len))
		return BOOL_TRUE;

	// If there is HOTKEY parameter then reinitialize whole hotkey table
	reset_hotkey_table(ctx);

	iter = faux_msg_init_param_iter(msg);
	while (faux_msg_get_param_each(
		&iter, &param_type, (void **)&param_data, &param_len)) {
		char *cmd = NULL;
		ssize_t code = -1;
		size_t key_len = 0;

		if (param_len < 3) // <key>'\0'<cmd>
			continue;
		if (KTP_PARAM_HOTKEY != param_type)
			continue;
		key_len = strlen(param_data); // Length of <key>
		if (key_len < 1)
			continue;
		code = vt100_hotkey_decode(param_data);
		if ((code < 0) || (code > VT100_HOTKEY_MAP_LEN))
			continue;
		cmd = faux_str_dupn(param_data + key_len + 1,
			param_len - key_len - 1);
		if (!cmd)
			continue;
		ctx->hotkeys[code] = cmd;
	}

	return BOOL_TRUE;
}


bool_t auth_ack_cb(ktp_session_t *ktp, const faux_msg_t *msg, void *udata)
{
	ctx_t *ctx = (ctx_t *)udata;
	int rc = -1;
	faux_error_t *error = NULL;

	process_prompt_param(ctx->tinyrl, msg);
	process_hotkey_param(ctx, msg);

	if (!ktp_session_retcode(ktp, &rc))
		rc = -1;
	error = ktp_session_error(ktp);
	if ((rc < 0) && (faux_error_len(error) > 0)) {
		faux_error_node_t *err_iter = faux_error_iter(error);
		const char *err = NULL;
		while ((err = faux_error_each(&err_iter)))
			fprintf(stderr, "Error: %s\n", err);
	}
	faux_error_free(error);

	// Operation is finished so restore stdin handler
	faux_eloop_add_fd(ktp_session_eloop(ktp), STDIN_FILENO, POLLIN,
		stdin_cb, ctx);

	// Happy compiler
	msg = msg;

	return BOOL_TRUE;
}


bool_t cmd_ack_cb(ktp_session_t *ktp, const faux_msg_t *msg, void *udata)
{
	ctx_t *ctx = (ctx_t *)udata;
	int rc = -1;
	faux_error_t *error = NULL;

	process_prompt_param(ctx->tinyrl, msg);
	process_hotkey_param(ctx, msg);

	if (!ktp_session_retcode(ktp, &rc))
		rc = -1;
	error = ktp_session_error(ktp);
	if ((rc < 0) && (faux_error_len(error) > 0)) {
		faux_error_node_t *err_iter = faux_error_iter(error);
		const char *err = NULL;
		while ((err = faux_error_each(&err_iter)))
			fprintf(stderr, "Error: %s\n", err);
	}
	faux_error_free(error);

	// Wait for pager
	if (ctx->pager_working == TRI_TRUE) {
		pclose(ctx->pager_pipe);
		ctx->pager_working = TRI_UNDEFINED;
		ctx->pager_pipe = NULL;
	}

	tinyrl_set_busy(ctx->tinyrl, BOOL_FALSE);
	if (!ktp_session_done(ktp))
		tinyrl_redisplay(ctx->tinyrl);

	// Operation is finished so restore stdin handler
	faux_eloop_add_fd(ktp_session_eloop(ktp), STDIN_FILENO, POLLIN,
		stdin_cb, ctx);

	// Happy compiler
	msg = msg;

	return BOOL_TRUE;
}


bool_t cmd_incompleted_ack_cb(ktp_session_t *ktp, const faux_msg_t *msg, void *udata)
{
	ctx_t *ctx = (ctx_t *)udata;

	// Interactive command. So restore stdin handler.
	if ((ktp_session_state(ktp) == KTP_SESSION_STATE_WAIT_FOR_CMD) &&
		KTP_STATUS_IS_INTERACTIVE(ktp_session_cmd_features(ktp))) {
		faux_eloop_add_fd(ktp_session_eloop(ktp), STDIN_FILENO, POLLIN,
			stdin_cb, ctx);
	}

	// Happy compiler
	msg = msg;

	return BOOL_TRUE;
}


static bool_t stdin_cb(faux_eloop_t *eloop, faux_eloop_type_e type,
	void *associated_data, void *udata)
{
	bool_t rc = BOOL_TRUE;
	ctx_t *ctx = (ctx_t *)udata;
	ktp_session_state_e state = KTP_SESSION_STATE_ERROR;
	faux_eloop_info_fd_t *info = (faux_eloop_info_fd_t *)associated_data;

	if (!ctx)
		return BOOL_FALSE;

	// Some errors or fd is closed so stop session
	if (info->revents & (POLLHUP | POLLERR | POLLNVAL))
		rc = BOOL_FALSE;

	state = ktp_session_state(ctx->ktp);

	// Standard klish command line
	if (state == KTP_SESSION_STATE_IDLE) {
		tinyrl_read(ctx->tinyrl);
		return rc;
	}

	// Interactive command
	if ((state == KTP_SESSION_STATE_WAIT_FOR_CMD) &&
		KTP_STATUS_IS_INTERACTIVE(ktp_session_cmd_features(ctx->ktp))) {
		int fd = fileno(tinyrl_istream(ctx->tinyrl));
		char buf[1024] = {};
		ssize_t bytes_readed = 0;

		while ((bytes_readed = read(fd, buf, sizeof(buf))) > 0) {
			ktp_session_stdin(ctx->ktp, buf, bytes_readed);
			if (bytes_readed != sizeof(buf))
				break;
		}
		return rc;
	}

	// Here the situation when input is not allowed. Remove stdin from
	// eloop waiting list. Else klish will get 100% CPU. Callbacks on
	// operation completions will restore this handler.
	faux_eloop_del_fd(eloop, STDIN_FILENO);

	// Happy compiler
	eloop = eloop;
	type = type;

	return rc;
}


static bool_t send_winch_notification(ctx_t *ctx)
{
	size_t width = 0;
	size_t height = 0;
	char *winsize = NULL;
	faux_msg_t *req = NULL;
	ktp_status_e status = KTP_STATUS_NONE;

	if (!ctx->tinyrl)
		return BOOL_FALSE;
	if (!ctx->ktp)
		return BOOL_FALSE;

	tinyrl_winsize(ctx->tinyrl, &width, &height);
	winsize = faux_str_sprintf("%lu %lu", width, height);

	req = ktp_msg_preform(KTP_NOTIFICATION, status);
	faux_msg_add_param(req, KTP_PARAM_WINCH, winsize, strlen(winsize));
	faux_str_free(winsize);
	faux_msg_send_async(req, ktp_session_async(ctx->ktp));
	faux_msg_free(req);

	return BOOL_TRUE;
}


static bool_t sigwinch_cb(faux_eloop_t *eloop, faux_eloop_type_e type,
	void *associated_data, void *udata)
{
	ctx_t *ctx = (ctx_t *)udata;

	if (!ctx)
		return BOOL_FALSE;

	send_winch_notification(ctx);

	// Happy compiler
	eloop = eloop;
	type = type;
	associated_data = associated_data;

	return BOOL_TRUE;
}


static bool_t tinyrl_key_enter(tinyrl_t *tinyrl, unsigned char key)
{
	const char *line = NULL;
	ctx_t *ctx = (ctx_t *)tinyrl_udata(tinyrl);
	faux_error_t *error = faux_error_new();

	tinyrl_line_to_hist(tinyrl);
	tinyrl_multi_crlf(tinyrl);
	tinyrl_reset_line_state(tinyrl);
	line = tinyrl_line(tinyrl);
	// Don't do anything on empty line
	if (faux_str_is_empty(line)) {
		faux_error_free(error);
		return BOOL_TRUE;
	}

	ktp_session_cmd(ctx->ktp, line, error, ctx->opts->dry_run);

	tinyrl_reset_line(tinyrl);
	tinyrl_set_busy(tinyrl, BOOL_TRUE);

	key = key; // Happy compiler

	return BOOL_TRUE;
}


static bool_t tinyrl_key_hotkey(tinyrl_t *tinyrl, unsigned char key)
{
	const char *line = NULL;
	ctx_t *ctx = (ctx_t *)tinyrl_udata(tinyrl);
	faux_error_t *error = NULL;

	if (key >= VT100_HOTKEY_MAP_LEN)
		return BOOL_TRUE;
	line = ctx->hotkeys[key];
	if (faux_str_is_empty(line))
		return BOOL_TRUE;

	error = faux_error_new();
	tinyrl_multi_crlf(tinyrl);
	tinyrl_reset_line_state(tinyrl);
	tinyrl_reset_line(tinyrl);

	ktp_session_cmd(ctx->ktp, line, error, ctx->opts->dry_run);

	tinyrl_set_busy(tinyrl, BOOL_TRUE);

	return BOOL_TRUE;
}


static bool_t tinyrl_key_tab(tinyrl_t *tinyrl, unsigned char key)
{
	char *line = NULL;
	ctx_t *ctx = (ctx_t *)tinyrl_udata(tinyrl);

	line = tinyrl_line_to_pos(tinyrl);
	ktp_session_completion(ctx->ktp, line, ctx->opts->dry_run);
	faux_str_free(line);

	tinyrl_set_busy(tinyrl, BOOL_TRUE);

	key = key; // Happy compiler

	return BOOL_TRUE;
}


static bool_t tinyrl_key_help(tinyrl_t *tinyrl, unsigned char key)
{
	char *line = NULL;
	ctx_t *ctx = (ctx_t *)tinyrl_udata(tinyrl);

	line = tinyrl_line_to_pos(tinyrl);
	// If "?" is quoted then it's not special hotkey.
	// Just insert it into the line.
	if (faux_str_unclosed_quotes(line, NULL)) {
		faux_str_free(line);
		return tinyrl_key_default(tinyrl, key);
	}

	ktp_session_help(ctx->ktp, line);
	faux_str_free(line);

	tinyrl_set_busy(tinyrl, BOOL_TRUE);

	key = key; // Happy compiler

	return BOOL_TRUE;
}


static void display_completions(const tinyrl_t *tinyrl, faux_list_t *completions,
	const char *prefix, size_t max)
{
	size_t width = tinyrl_width(tinyrl);
	size_t cols = 0;
	faux_list_node_t *iter = NULL;
	faux_list_node_t *node = NULL;
	size_t prefix_len = 0;
	size_t cols_filled = 0;

	if (prefix)
		prefix_len = strlen(prefix);

	// Find out column and rows number
	if (max < width)
		cols = (width + 1) / (prefix_len + max + 1); // For a space between words
	else
		cols = 1;

	iter = faux_list_head(completions);
	while ((node = faux_list_each_node(&iter))) {
		char *compl = (char *)faux_list_data(node);
		tinyrl_printf(tinyrl, "%*s%s",
			(prefix_len + max + 1 - strlen(compl)),
			prefix ? prefix : "",
			compl);
		cols_filled++;
		if ((cols_filled >= cols) || (node == faux_list_tail(completions))) {
			cols_filled = 0;
			tinyrl_crlf(tinyrl);
		}
	}
}


bool_t completion_ack_cb(ktp_session_t *ktp, const faux_msg_t *msg, void *udata)
{
	ctx_t *ctx = (ctx_t *)udata;
	faux_list_node_t *iter = NULL;
	uint32_t param_len = 0;
	char *param_data = NULL;
	uint16_t param_type = 0;
	char *prefix = NULL;
	faux_list_t *completions = NULL;
	size_t completions_num = 0;
	size_t max_compl_len = 0;

	tinyrl_set_busy(ctx->tinyrl, BOOL_FALSE);

	process_prompt_param(ctx->tinyrl, msg);

	prefix = faux_msg_get_str_param_by_type(msg, KTP_PARAM_PREFIX);

	completions = faux_list_new(FAUX_LIST_UNSORTED, FAUX_LIST_NONUNIQUE,
		NULL, NULL, (void (*)(void *))faux_str_free);

	iter = faux_msg_init_param_iter(msg);
	while (faux_msg_get_param_each(&iter, &param_type, (void **)&param_data, &param_len)) {
		char *compl = NULL;
		if (KTP_PARAM_LINE != param_type)
			continue;
		compl = faux_str_dupn(param_data, param_len);
		faux_list_add(completions, compl);
		if (param_len > max_compl_len)
			max_compl_len = param_len;
	}

	completions_num = faux_list_len(completions);

	// Single possible completion
	if (1 == completions_num) {
		char *compl = (char *)faux_list_data(faux_list_head(completions));
		tinyrl_line_insert(ctx->tinyrl, compl, strlen(compl));
		// Add space after completion
		tinyrl_line_insert(ctx->tinyrl, " ", 1);
		tinyrl_redisplay(ctx->tinyrl);

	// Multi possible completions
	} else if (completions_num > 1) {
		faux_list_node_t *eq_iter = NULL;
		size_t eq_part = 0;
		char *str = NULL;
		char *compl = NULL;

		// Try to find equal part for all possible completions
		eq_iter = faux_list_head(completions);
		str = (char *)faux_list_data(eq_iter);
		eq_part = strlen(str);
		eq_iter = faux_list_next_node(eq_iter);

		while ((compl = (char *)faux_list_each(&eq_iter)) && (eq_part > 0)) {
			size_t cur_eq = 0;
			cur_eq = tinyrl_equal_part(ctx->tinyrl, str, compl);
			if (cur_eq < eq_part)
				eq_part = cur_eq;
		}

		// The equal part was found
		if (eq_part > 0) {
			tinyrl_line_insert(ctx->tinyrl, str, eq_part);
			tinyrl_redisplay(ctx->tinyrl);

		// There is no equal part for all completions
		} else {
			tinyrl_multi_crlf(ctx->tinyrl);
			tinyrl_reset_line_state(ctx->tinyrl);
			display_completions(ctx->tinyrl, completions,
				prefix, max_compl_len);
			tinyrl_redisplay(ctx->tinyrl);
		}
	}

	faux_list_free(completions);
	faux_str_free(prefix);

	// Operation is finished so restore stdin handler
	faux_eloop_add_fd(ktp_session_eloop(ktp), STDIN_FILENO, POLLIN,
		stdin_cb, ctx);

	// Happy compiler
	ktp = ktp;
	msg = msg;

	return BOOL_TRUE;
}


static void display_help(const tinyrl_t *tinyrl, faux_list_t *help_list,
	size_t max)
{
	faux_list_node_t *iter = NULL;
	faux_list_node_t *node = NULL;

	iter = faux_list_head(help_list);
	while ((node = faux_list_each_node(&iter))) {
		help_t *help = (help_t *)faux_list_data(node);
		tinyrl_printf(tinyrl, "  %s%*s%s\n",
			help->prefix,
			(max + 2 - strlen(help->prefix)),
			" ",
			help->line);
	}
}


bool_t help_ack_cb(ktp_session_t *ktp, const faux_msg_t *msg, void *udata)
{
	ctx_t *ctx = (ctx_t *)udata;
	faux_list_t *help_list = NULL;
	faux_list_node_t *iter = NULL;
	uint32_t param_len = 0;
	char *param_data = NULL;
	uint16_t param_type = 0;
	size_t max_prefix_len = 0;

	tinyrl_set_busy(ctx->tinyrl, BOOL_FALSE);

	process_prompt_param(ctx->tinyrl, msg);

	help_list = faux_list_new(FAUX_LIST_SORTED, FAUX_LIST_NONUNIQUE,
		help_compare, help_kcompare, help_free);

	// Wait for PREFIX - LINE pairs
	iter = faux_msg_init_param_iter(msg);
	while (faux_msg_get_param_each(&iter, &param_type, (void **)&param_data,
		&param_len)) {
		char *prefix_str = NULL;
		char *line_str = NULL;
		help_t *help = NULL;
		size_t prefix_len = 0;

		// Get PREFIX
		if (KTP_PARAM_PREFIX != param_type)
			continue;
		prefix_str = faux_str_dupn(param_data, param_len);
		prefix_len = param_len;

		// Get LINE
		if (!faux_msg_get_param_each(&iter, &param_type,
			(void **)&param_data, &param_len) ||
			(KTP_PARAM_LINE != param_type)) {
			faux_str_free(prefix_str);
			break;
		}
		line_str = faux_str_dupn(param_data, param_len);

		help = help_new(prefix_str, line_str);
		faux_list_add(help_list, help);
		if (prefix_len > max_prefix_len)
			max_prefix_len = prefix_len;
	}

	if (faux_list_len(help_list) > 0) {
		tinyrl_multi_crlf(ctx->tinyrl);
		tinyrl_reset_line_state(ctx->tinyrl);
		display_help(ctx->tinyrl, help_list, max_prefix_len);
		tinyrl_redisplay(ctx->tinyrl);
	}

	faux_list_free(help_list);

	// Operation is finished so restore stdin handler
	faux_eloop_add_fd(ktp_session_eloop(ktp), STDIN_FILENO, POLLIN,
		stdin_cb, ctx);

	ktp = ktp; // happy compiler

	return BOOL_TRUE;
}


static bool_t ktp_sync_auth(ktp_session_t *ktp, int *retcode,
	faux_error_t *error)
{
	if (!ktp_session_auth(ktp, error))
		return BOOL_FALSE;

	faux_eloop_loop(ktp_session_eloop(ktp));

	return ktp_session_retcode(ktp, retcode);
}


static bool_t interactive_stdout_cb(ktp_session_t *ktp, const char *line, size_t len,
	void *udata)
{
	ctx_t *ctx = (ctx_t *)udata;

	assert(ctx);

	// Start pager if necessary
	if (
		ctx->opts->pager_enabled && // Pager enabled within config file
		(ctx->pager_working == TRI_UNDEFINED) && // Pager is not working
		!(ktp_session_cmd_features(ktp) & KTP_STATUS_INTERACTIVE) // Non interactive command
		) {

		ctx->pager_pipe = popen(ctx->opts->pager, "we");
		if (!ctx->pager_pipe)
			ctx->pager_working = TRI_FALSE; // Indicates can't start
		else
			ctx->pager_working = TRI_TRUE;
	}

	// Write to pager's pipe if pager is really working
	if (ctx->pager_working == TRI_TRUE) {
		if (faux_write_block(fileno(ctx->pager_pipe), line, len) <= 0) {
			// If we can't write to pager pipe then try stdout.
			if (faux_write_block(STDOUT_FILENO, line, len) < 0)
				return BOOL_FALSE;
			return BOOL_TRUE; // Don't break the loop
		}
	} else {
		if (faux_write_block(STDOUT_FILENO, line, len) < 0)
			return BOOL_FALSE;
	}

	return BOOL_TRUE;
}
