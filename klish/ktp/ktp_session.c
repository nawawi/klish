#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <faux/str.h>
#include <klish/ktp_session.h>


typedef enum {
	KTP_SESSION_STATE_DISCONNECTED = 'd',
	KTP_SESSION_STATE_UNAUTHORIZED = 'a',
	KTP_SESSION_STATE_IDLE = 'i',
	KTP_SESSION_STATE_WAIT_FOR_COMPLETION = 'v',
	KTP_SESSION_STATE_WAIT_FOR_HELP = 'h',
	KTP_SESSION_STATE_WAIT_FOR_CMD = 'c',
} ktp_session_state_e;

struct ktp_session_s {
	ktp_session_state_e state;
	faux_async_t *async;
	faux_hdr_t *hdr; // Service var: engine will receive header and then msg
	bool_t done;
	faux_eloop_t *eloop;
};


static bool_t stop_loop_ev(faux_eloop_t *eloop, faux_eloop_type_e type,
	void *associated_data, void *user_data);


ktp_session_t *ktp_session_new(int sock)
{
	ktp_session_t *ktp = NULL;

	if (sock < 0)
		return NULL;

	ktp = faux_zmalloc(sizeof(*ktp));
	assert(ktp);
	if (!ktp)
		return NULL;

	// Init
	ktp->state = KTP_SESSION_STATE_UNAUTHORIZED;
	ktp->done = BOOL_FALSE;

	// Event loop
	ktp->eloop = faux_eloop_new(NULL);

	// Async object
	ktp->async = faux_async_new(sock);
	assert(ktp->async);
	// Receive message header first
	faux_async_set_read_limits(ktp->async,
		sizeof(faux_hdr_t), sizeof(faux_hdr_t));
//	faux_async_set_read_cb(ktp->async, ktpd_session_read_cb, ktpd);
	ktp->hdr = NULL;
	faux_async_set_stall_cb(ktp->async, ktp_stall_cb, ktp->eloop);

	// Event loop handlers
	faux_eloop_add_signal(ktp->eloop, SIGINT, stop_loop_ev, ktp);
	faux_eloop_add_signal(ktp->eloop, SIGTERM, stop_loop_ev, ktp);
	faux_eloop_add_signal(ktp->eloop, SIGQUIT, stop_loop_ev, ktp);
	faux_eloop_add_fd(ktp->eloop, ktp_session_fd(ktp), POLLIN,
		ktp_peer_ev, ktp);

	return ktp;
}


void ktp_session_free(ktp_session_t *ktp)
{
	if (!ktp)
		return;

	faux_free(ktp->hdr);
	close(ktp_session_fd(ktp));
	faux_async_free(ktp->async);
	faux_eloop_free(ktp->eloop);
	faux_free(ktp);
}


bool_t ktp_session_done(const ktp_session_t *ktp)
{
	assert(ktp);
	if (!ktp)
		return BOOL_TRUE; // Done flag

	return ktp->done;
}


bool_t ktp_session_set_done(ktp_session_t *ktp, bool_t done)
{
	assert(ktp);
	if (!ktp)
		return BOOL_FALSE;

	ktp->done = done;

	return BOOL_TRUE;
}


bool_t ktp_session_connected(ktp_session_t *ktp)
{
	assert(ktp);
	if (!ktp)
		return BOOL_FALSE;
	if (KTP_SESSION_STATE_DISCONNECTED == ktp->state)
		return BOOL_FALSE;

	return BOOL_TRUE;
}


int ktp_session_fd(const ktp_session_t *ktp)
{
	assert(ktp);
	if (!ktp)
		return BOOL_FALSE;

	return faux_async_fd(ktp->async);
}


#if 0
static void ktp_session_bad_socket(ktp_session_t *ktp)
{
	assert(ktp);
	if (!ktp)
		return;

	ktp->state = KTP_SESSION_STATE_DISCONNECTED;
}
#endif


static bool_t stop_loop_ev(faux_eloop_t *eloop, faux_eloop_type_e type,
	void *associated_data, void *user_data)
{
	ktp_session_t *ktp = (ktp_session_t *)user_data;

	if (!ktp)
		return BOOL_FALSE;

	ktp_session_set_done(ktp, BOOL_TRUE);

	// Happy compiler
	eloop = eloop;
	type = type;
	associated_data = associated_data;

	return BOOL_FALSE; // Stop Event Loop
}


bool_t ktp_session_req_cmd(ktp_session_t *ktp, const char *line, int *retcode)
{
	faux_eloop_t *eloop = NULL;

	assert(ktp);
	if (!ktp)
		return BOOL_FALSE;

	faux_eloop_loop(eloop);

	line = line;
	retcode = retcode;

	return BOOL_TRUE;
}
