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

#if 0
static void ktpd_session_bad_socket(ktpd_session_t *session)
{
	assert(session);
	if (!session)
		return;

	session->state = KTPD_SESSION_STATE_DISCONNECTED;
}
#endif
