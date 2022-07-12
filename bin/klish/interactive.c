#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>

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
static bool_t stdin_cb(faux_eloop_t *eloop, faux_eloop_type_e type,
	void *associated_data, void *user_data);

// Keys
static bool_t tinyrl_key_enter(tinyrl_t *tinyrl, unsigned char key);


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
	tinyrl_bind_key(tinyrl, KEY_CR, tinyrl_key_enter);
	tinyrl_redisplay(tinyrl);

	ctx.ktp = ktp;
	ctx.tinyrl = tinyrl;
	ctx.opts = opts;

	// Don't stop interactive loop on each answer
	ktp_session_set_stop_on_answer(ktp, BOOL_FALSE);
	ktp_session_set_cb(ktp, KTP_SESSION_CB_CMD_ACK, cmd_ack_cb, &ctx);
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
	ktp = ktp;
	msg = msg;
	udata = udata;

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
