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
#include <faux/async.h>
#include <faux/msg.h>
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


static bool_t ktpd_session_dispatch(ktpd_session_t *session, faux_msg_t *msg)
{
	assert(session);
	if (!session)
		return BOOL_FALSE;
	assert(msg);
	if (!msg)
		return BOOL_FALSE;

	printf("Dispatch %d\n", faux_msg_get_len(msg));

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


ktpd_session_t *ktpd_session_new(int sock)
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
	faux_session_stall_cb_f stall_cb, void *user_data)
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
