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
#include <syslog.h>

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
static bool_t ktp_session_read_cb(faux_async_t *async,
	faux_buf_t *buf, size_t len, void *user_data);


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
	faux_async_set_read_cb(ktp->async, ktp_session_read_cb, ktp);
	ktp->hdr = NULL;
	faux_async_set_stall_cb(ktp->async, ktp_stall_cb, ktp->eloop);

	// Event loop handlers
	faux_eloop_add_signal(ktp->eloop, SIGINT, stop_loop_ev, ktp);
	faux_eloop_add_signal(ktp->eloop, SIGTERM, stop_loop_ev, ktp);
	faux_eloop_add_signal(ktp->eloop, SIGQUIT, stop_loop_ev, ktp);
	faux_eloop_add_fd(ktp->eloop, ktp_session_fd(ktp), POLLIN,
		ktp_peer_ev, ktp->async);

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


static bool_t ktp_session_dispatch(ktp_session_t *ktp, faux_msg_t *msg)
{
	uint16_t cmd = 0;

	assert(ktp);
	if (!ktp)
		return BOOL_FALSE;
	assert(msg);
	if (!msg)
		return BOOL_FALSE;

	cmd = faux_msg_get_cmd(msg);
	switch (cmd) {
	case KTP_CMD_ACK:
		{
		int retcode = -1;
		uint8_t *retcode8bit = NULL;
		if (faux_msg_get_param_by_type(msg, KTP_PARAM_RETCODE,
			(void **)&retcode8bit, NULL)) {
			retcode = (int)(*retcode8bit);
			printf("Retcode: %d\n", retcode);
		}
		return BOOL_FALSE;
		}
//		ktpd_session_process_cmd(ktpd, msg);
		break;
	case KTP_STDOUT:
		{
		char *line = NULL;
		unsigned int len = 0;
		if (faux_msg_get_param_by_type(msg, KTP_PARAM_LINE,
			(void **)&line, &len)) {
			write(STDOUT_FILENO, line, len);
		}
		}
//		ktpd_session_process_completion(ktpd, msg);
		break;
	case KTP_HELP:
//		ktpd_session_process_help(ktpd, msg);
		break;
	default:
		syslog(LOG_WARNING, "Unsupported command: 0x%04u\n", cmd);
		break;
	}

	return BOOL_TRUE;
}


static bool_t ktp_session_read_cb(faux_async_t *async,
	faux_buf_t *buf, size_t len, void *user_data)
{
	ktp_session_t *ktp = (ktp_session_t *)user_data;
	faux_msg_t *completed_msg = NULL;
	char *data = NULL;

	assert(async);
	assert(buf);
	assert(ktp);

	// Linearize buffer
	data = malloc(len);
	faux_buf_read(buf, data, len);

	// Receive header
	if (!ktp->hdr) {
		size_t whole_len = 0;
		size_t msg_wo_hdr = 0;

		ktp->hdr = (faux_hdr_t *)data;
		// Check for broken header
		if (!ktp_check_header(ktp->hdr)) {
			faux_free(ktp->hdr);
			ktp->hdr = NULL;
			return BOOL_FALSE;
		}

		whole_len = faux_hdr_len(ktp->hdr);
		// msg_wo_hdr >= 0 because ktp_check_header() validates whole_len
		msg_wo_hdr = whole_len - sizeof(faux_hdr_t);
		// Plan to receive message body
		if (msg_wo_hdr > 0) {
			faux_async_set_read_limits(async,
				msg_wo_hdr, msg_wo_hdr);
			return BOOL_TRUE;
		}
		// Here message is completed (msg body has zero length)
		completed_msg = faux_msg_deserialize_parts(ktp->hdr, NULL, 0);

	// Receive message body
	} else {
		completed_msg = faux_msg_deserialize_parts(ktp->hdr, data, len);
		faux_free(data);
	}

	// Plan to receive msg header
	faux_async_set_read_limits(ktp->async,
		sizeof(faux_hdr_t), sizeof(faux_hdr_t));
	faux_free(ktp->hdr);
	ktp->hdr = NULL; // Ready to recv new header

	// Here message is completed
	ktp_session_dispatch(ktp, completed_msg);
	faux_msg_free(completed_msg);

	// Session status can be changed while parsing
	if (ktp_session_done(ktp))
		return BOOL_FALSE;

	return BOOL_TRUE;
}


bool_t ktp_session_req_cmd(ktp_session_t *ktp, const char *line, int *retcode)
{
	faux_msg_t *req = NULL;

	assert(ktp);
	if (!ktp)
		return BOOL_FALSE;

	req = ktp_msg_preform(KTP_CMD, KTP_STATUS_NONE);
	faux_msg_add_param(req, KTP_PARAM_LINE, line, strlen(line));
	faux_msg_send_async(req, ktp->async);
	faux_msg_free(req);


	faux_eloop_loop(ktp->eloop);

	line = line;
	retcode = retcode;

	return BOOL_TRUE;
}
