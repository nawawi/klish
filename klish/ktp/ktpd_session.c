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
#include <klish/ktp_session.h>

#include "private.h"


static bool_t ktpd_session_read_cb(faux_async_t *async,
	void *data, size_t len, void *user_data)
{
	ktpd_session_t *session = (ktpd_session_t *)user_data;

	assert(async);
	assert(data);
	assert(session);

	printf("Read cb %lu\n", len);
	faux_free(data);

	async = async; // Happy compiler

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
	faux_async_set_read_cb(session->async, ktpd_session_read_cb, session);

	return session;
}


void ktpd_session_free(ktpd_session_t *session)
{
	if (!session)
		return;

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
