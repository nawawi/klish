#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>

#include <faux/faux.h>
#include <faux/eloop.h>
#include <klish/ktp.h>
#include <klish/ktp_session.h>
#include <tinyrl/tinyrl.h>


// Context for main loop
typedef struct ctx_s {
	ktp_session_t *ktp;
	tinyrl_t *tinyrl;
} ctx_t;


bool_t cmd_ack_cb(ktp_session_t *ktp, const faux_msg_t *msg, void *udata);
static bool_t stdin_cb(faux_eloop_t *eloop, faux_eloop_type_e type,
	void *associated_data, void *user_data);


int klish_interactive_shell(ktp_session_t *ktp)
{
	ctx_t ctx = {};
	faux_eloop_t *eloop = NULL;
	tinyrl_t *tinyrl = NULL;
	int stdin_flags = 0;

	assert(ktp);
	if (!ktp)
		return -1;

	// Set stdin to O_NONBLOCK mode
	stdin_flags = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, stdin_flags | O_NONBLOCK);

	tinyrl = tinyrl_new(stdin, stdout, 0, NULL);

	ctx.ktp = ktp;
	ctx.tinyrl = tinyrl;

	// Don't stop interactive loop on each answer
	ktp_session_set_stop_on_answer(ktp, BOOL_FALSE);
	ktp_session_set_cb(ktp, KTP_SESSION_CB_CMD_ACK, cmd_ack_cb, &ctx);
	eloop = ktp_session_eloop(ktp);
	faux_eloop_add_fd(eloop, STDIN_FILENO, POLLIN, stdin_cb, &ctx);
	faux_eloop_loop(eloop);

	// Cleanup
	tinyrl_free(tinyrl);

	// Restore stdin mode
	fcntl(STDIN_FILENO, F_SETFL, stdin_flags);

	return 0;
}


bool_t cmd_ack_cb(ktp_session_t *ktp, const faux_msg_t *msg, void *udata)
{
	printf("CMD ACK CB\n");

	// Happy compiler
	ktp = ktp;
	msg = msg;
	udata = udata;

	ktp_session_set_done(ktp, BOOL_TRUE);

	return BOOL_TRUE;
}


static bool_t stdin_cb(faux_eloop_t *eloop, faux_eloop_type_e type,
	void *associated_data, void *udata)
{
	ctx_t *ctx = (ctx_t *)udata;

	ktp_session_cmd(ctx->ktp, "cmd", NULL, BOOL_FALSE);

	return BOOL_TRUE;
}

