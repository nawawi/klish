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
#include <faux/async.h>
#include <faux/msg.h>
#include <klish/ksession.h>
#include <klish/ktp.h>
#include <klish/ktp_session.h>

#include "private.h"


static bool_t check_ktp_header(faux_hdr_t *hdr)
{
	assert(hdr);
	if (!hdr)
		return BOOL_FALSE;

	if (faux_hdr_magic(hdr) != KTP_MAGIC)
		return BOOL_FALSE;
	if (faux_hdr_major(hdr) != KTP_MAJOR)
		return BOOL_FALSE;
	if (faux_hdr_minor(hdr) != KTP_MINOR)
		return BOOL_FALSE;
	if (faux_hdr_len(hdr) < (int)sizeof(*hdr))
		return BOOL_FALSE;

	return BOOL_TRUE;
}


static bool_t ktpd_session_send_error(ktpd_session_t *session,
	ktp_cmd_e cmd, const char *error)
{
	faux_msg_t *msg = NULL;

	assert(session);
	if (!session)
		return BOOL_FALSE;

	msg = ktp_msg_preform(cmd, KTP_STATUS_ERROR);
	if (error)
		faux_msg_add_param(msg, KTP_PARAM_ERROR, error, strlen(error));
	faux_msg_send_async(msg, session->async);
	faux_msg_free(msg);

	return BOOL_TRUE;
}


static bool_t ktpd_session_process_cmd(ktpd_session_t *session, faux_msg_t *msg)
{
	char *line = NULL;
	faux_msg_t *ack = NULL;
//	kpargv_t *pargv = NULL;
	ktp_cmd_e cmd = KTP_CMD_ACK;
	kexec_t *exec = NULL;
	faux_error_t *error = NULL;

	assert(session);
	assert(msg);

	// Get line from message
	if (!(line = faux_msg_get_str_param_by_type(msg, KTP_PARAM_LINE))) {
		ktpd_session_send_error(session, cmd,
			"The line is not specified");
		return BOOL_FALSE;
	}

	// Parsing
	error = faux_error_new();
	exec = ksession_parse_for_exec(session->ksession, line, error);
	faux_str_free(line);

	if (exec) {
		kexec_contexts_node_t *iter = kexec_contexts_iter(exec);
		kcontext_t *context = NULL;
		while ((context = kexec_contexts_each(&iter))) {
			kpargv_debug(kcontext_pargv(context));
		}
	} else {
		faux_error_show(error);
	}


//	ktpd_session_exec(session, exec);

//	kpargv_debug(pargv);
//	if (kpargv_status(pargv) != KPARSE_OK) {
//		char *error = NULL;
//		error = faux_str_sprintf("Can't parse line: %s",
//			kpargv_status_str(pargv));
//		kpargv_free(pargv);
//		ktpd_session_send_error(session, cmd, error);
//		return BOOL_FALSE;
//	}
//
//	kpargv_free(pargv);
	kexec_free(exec);
	faux_error_free(error);

	// Send ACK message
	ack = ktp_msg_preform(cmd, KTP_STATUS_NONE);
	faux_msg_send_async(ack, session->async);
	faux_msg_free(ack);

	return BOOL_TRUE;
}


static bool_t ktpd_session_process_completion(ktpd_session_t *session, faux_msg_t *msg)
{
	char *line = NULL;
	faux_msg_t *ack = NULL;
	kpargv_t *pargv = NULL;
	ktp_cmd_e cmd = KTP_COMPLETION_ACK;

	assert(session);
	assert(msg);

	// Get line from message
	if (!(line = faux_msg_get_str_param_by_type(msg, KTP_PARAM_LINE))) {
		ktpd_session_send_error(session, cmd, NULL);
		return BOOL_FALSE;
	}

	// Parsing
	pargv = ksession_parse_for_completion(session->ksession, line);
	faux_str_free(line);
	if (!pargv) {
		ktpd_session_send_error(session, cmd, NULL);
		return BOOL_FALSE;
	}
	kpargv_debug(pargv);

	kpargv_free(pargv);

	// Send ACK message
	ack = ktp_msg_preform(cmd, KTP_STATUS_NONE);
	faux_msg_send_async(ack, session->async);
	faux_msg_free(ack);

	return BOOL_TRUE;
}


static bool_t ktpd_session_process_help(ktpd_session_t *session, faux_msg_t *msg)
{
	char *line = NULL;
	faux_msg_t *ack = NULL;
//	kpargv_t *pargv = NULL;
	ktp_cmd_e cmd = KTP_HELP_ACK;

	assert(session);
	assert(msg);

	// Get line from message
	if (!(line = faux_msg_get_str_param_by_type(msg, KTP_PARAM_LINE))) {
		ktpd_session_send_error(session, cmd, NULL);
		return BOOL_FALSE;
	}

/*	// Parsing
	pargv = ksession_parse_line(session->ksession, line, KPURPOSE_HELP);
	faux_str_free(line);
	kpargv_free(pargv);
*/
	// Send ACK message
	ack = ktp_msg_preform(cmd, KTP_STATUS_NONE);
	faux_msg_send_async(ack, session->async);
	faux_msg_free(ack);

	return BOOL_TRUE;
}


static bool_t ktpd_session_dispatch(ktpd_session_t *session, faux_msg_t *msg)
{
	uint16_t cmd = 0;

	assert(session);
	if (!session)
		return BOOL_FALSE;
	assert(msg);
	if (!msg)
		return BOOL_FALSE;

	cmd = faux_msg_get_cmd(msg);
	switch (cmd) {
	case KTP_CMD:
		ktpd_session_process_cmd(session, msg);
		break;
	case KTP_COMPLETION:
		ktpd_session_process_completion(session, msg);
		break;
	case KTP_HELP:
		ktpd_session_process_help(session, msg);
		break;
	default:
		syslog(LOG_WARNING, "Unsupported command: 0x%04u\n", cmd);
		break;
	}

	return BOOL_TRUE;
}


/** @brief Low-level function to receive KTP message.
 *
 * Firstly function gets the header of message. Then it checks and parses
 * header and find out the length of whole message. Then it receives the rest
 * of message.
 */
static bool_t ktpd_session_read_cb(faux_async_t *async,
	void *data, size_t len, void *user_data)
{
	ktpd_session_t *session = (ktpd_session_t *)user_data;
	faux_msg_t *completed_msg = NULL;

	assert(async);
	assert(data);
	assert(session);

	// Receive header
	if (!session->hdr) {
		size_t whole_len = 0;
		size_t msg_wo_hdr = 0;

		session->hdr = (faux_hdr_t *)data;
		// Check for broken header
		if (!check_ktp_header(session->hdr)) {
			faux_free(session->hdr);
			session->hdr = NULL;
			return BOOL_FALSE;
		}

		whole_len = faux_hdr_len(session->hdr);
		// msg_wo_hdr >= 0 because check_ktp_header() validates whole_len
		msg_wo_hdr = whole_len - sizeof(faux_hdr_t);
		// Plan to receive message body
		if (msg_wo_hdr > 0) {
			faux_async_set_read_limits(async,
				msg_wo_hdr, msg_wo_hdr);
			return BOOL_TRUE;
		}
		// Here message is completed (msg body has zero length)
		completed_msg = faux_msg_deserialize_parts(session->hdr, NULL, 0);

	// Receive message body
	} else {
		completed_msg = faux_msg_deserialize_parts(session->hdr, data, len);
		faux_free(data);
	}

	// Plan to receive msg header
	faux_async_set_read_limits(session->async,
		sizeof(faux_hdr_t), sizeof(faux_hdr_t));
	faux_free(session->hdr);
	session->hdr = NULL; // Ready to recv new header

	// Here message is completed
	ktpd_session_dispatch(session, completed_msg);
	faux_msg_free(completed_msg);

	return BOOL_TRUE;
}


static bool_t ktpd_session_stall_cb(faux_async_t *async,
	size_t len, void *user_data)
{
	ktpd_session_t *session = (ktpd_session_t *)user_data;

	assert(async);
	assert(session);

	if (!session->stall_cb)
		return BOOL_TRUE;

	session->stall_cb(session, session->stall_udata);

	async = async; // Happy compiler
	len = len; // Happy compiler

	return BOOL_TRUE;
}


ktpd_session_t *ktpd_session_new(int sock, const kscheme_t *scheme,
	const char *start_entry)
{
	ktpd_session_t *session = NULL;

	if (sock < 0)
		return NULL;

	session = faux_zmalloc(sizeof(*session));
	assert(session);
	if (!session)
		return NULL;

	// Init
	session->state = KTPD_SESSION_STATE_NOT_AUTHORIZED;
	session->ksession = ksession_new(scheme, start_entry);
	assert(session->ksession);
	session->async = faux_async_new(sock);
	assert(session->async);
	// Receive message header first
	faux_async_set_read_limits(session->async,
		sizeof(faux_hdr_t), sizeof(faux_hdr_t));
	faux_async_set_read_cb(session->async, ktpd_session_read_cb, session);
	session->hdr = NULL;

	return session;
}


void ktpd_session_free(ktpd_session_t *session)
{
	if (!session)
		return;

	ksession_free(session->ksession);
	faux_free(session->hdr);
	close(ktpd_session_fd(session));
	faux_async_free(session->async);
	faux_free(session);
}


bool_t ktpd_session_connected(ktpd_session_t *session)
{
	assert(session);
	if (!session)
		return BOOL_FALSE;
	if (KTPD_SESSION_STATE_DISCONNECTED == session->state)
		return BOOL_FALSE;

	return BOOL_TRUE;
}


int ktpd_session_fd(const ktpd_session_t *session)
{
	assert(session);
	if (!session)
		return BOOL_FALSE;

	return faux_async_fd(session->async);
}


bool_t ktpd_session_async_in(ktpd_session_t *session)
{
	assert(session);
	if (!session)
		return BOOL_FALSE;
	if (!ktpd_session_connected(session))
		return BOOL_FALSE;

	if (faux_async_in(session->async) < 0)
		return BOOL_FALSE;

	return BOOL_TRUE;
}


bool_t ktpd_session_async_out(ktpd_session_t *session)
{
	assert(session);
	if (!session)
		return BOOL_FALSE;
	if (!ktpd_session_connected(session))
		return BOOL_FALSE;

	if (faux_async_out(session->async) < 0)
		return BOOL_FALSE;

	return BOOL_TRUE;
}


void ktpd_session_set_stall_cb(ktpd_session_t *session,
	ktpd_session_stall_cb_fn stall_cb, void *user_data)
{
	assert(session);
	if (!session)
		return;

	session->stall_cb = stall_cb;
	session->stall_udata = user_data;
	faux_async_set_stall_cb(session->async, ktpd_session_stall_cb, session);
}


#if 0
static void ktpd_session_bad_socket(ktpd_session_t *session)
{
	assert(session);
	if (!session)
		return;

	session->state = KTPD_SESSION_STATE_DISCONNECTED;
}
#endif
