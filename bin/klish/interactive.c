#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

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
} ctx_t;


bool_t cmd_ack_cb(ktp_session_t *ktp, const faux_msg_t *msg, void *udata);
bool_t completion_ack_cb(ktp_session_t *ktp, const faux_msg_t *msg, void *udata);
static bool_t stdin_cb(faux_eloop_t *eloop, faux_eloop_type_e type,
	void *associated_data, void *user_data);

// Keys
static bool_t tinyrl_key_enter(tinyrl_t *tinyrl, unsigned char key);
static bool_t tinyrl_key_tab(tinyrl_t *tinyrl, unsigned char key);
static bool_t tinyrl_key_help(tinyrl_t *tinyrl, unsigned char key);


int klish_interactive_shell(ktp_session_t *ktp, struct options *opts)
{
	ctx_t ctx = {};
	faux_eloop_t *eloop = NULL;
	tinyrl_t *tinyrl = NULL;
	int stdin_flags = 0;
	char *hist_path = NULL;

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
	tinyrl_bind_key(tinyrl, '\n', tinyrl_key_enter);
	tinyrl_bind_key(tinyrl, '\r', tinyrl_key_enter);
	tinyrl_bind_key(tinyrl, '\t', tinyrl_key_tab);
	tinyrl_bind_key(tinyrl, '?', tinyrl_key_help);
	tinyrl_redisplay(tinyrl);

	ctx.ktp = ktp;
	ctx.tinyrl = tinyrl;
	ctx.opts = opts;

	// Don't stop interactive loop on each answer
	ktp_session_set_stop_on_answer(ktp, BOOL_FALSE);
	ktp_session_set_cb(ktp, KTP_SESSION_CB_CMD_ACK, cmd_ack_cb, &ctx);
	ktp_session_set_cb(ktp, KTP_SESSION_CB_COMPLETION_ACK, completion_ack_cb, &ctx);
	eloop = ktp_session_eloop(ktp);
	faux_eloop_add_fd(eloop, STDIN_FILENO, POLLIN, stdin_cb, &ctx);
	faux_eloop_loop(eloop);

	// Cleanup
	if (tinyrl_busy(tinyrl))
		faux_error_free(ktp_session_error(ktp));
	tinyrl_free(tinyrl);

	// Restore stdin mode
	fcntl(STDIN_FILENO, F_SETFL, stdin_flags);

	return 0;
}


bool_t cmd_ack_cb(ktp_session_t *ktp, const faux_msg_t *msg, void *udata)
{
	ctx_t *ctx = (ctx_t *)udata;
	int rc = -1;
	faux_error_t *error = NULL;

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

	tinyrl_set_busy(ctx->tinyrl, BOOL_FALSE);
	if (!ktp_session_done(ktp))
		tinyrl_redisplay(ctx->tinyrl);

	// Happy compiler
	msg = msg;

	return BOOL_TRUE;
}


static bool_t stdin_cb(faux_eloop_t *eloop, faux_eloop_type_e type,
	void *associated_data, void *udata)
{
	ctx_t *ctx = (ctx_t *)udata;

	tinyrl_read(ctx->tinyrl);

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

	return BOOL_TRUE;
}


static bool_t tinyrl_key_tab(tinyrl_t *tinyrl, unsigned char key)
{
	const char *line = NULL;
	ctx_t *ctx = (ctx_t *)tinyrl_udata(tinyrl);

	line = tinyrl_line(tinyrl);
	ktp_session_completion(ctx->ktp, line, ctx->opts->dry_run);

	tinyrl_set_busy(tinyrl, BOOL_TRUE);

	return BOOL_TRUE;
}


static bool_t tinyrl_key_help(tinyrl_t *tinyrl, unsigned char key)
{
	const char *line = NULL;
	ctx_t *ctx = (ctx_t *)tinyrl_udata(tinyrl);

	line = tinyrl_line(tinyrl);
	ktp_session_help(ctx->ktp, line);

	tinyrl_set_busy(tinyrl, BOOL_TRUE);

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

	// Happy compiler
	ktp = ktp;
	msg = msg;

	return BOOL_TRUE;
}
