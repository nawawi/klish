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

#include "private.h"

#define USOCK_PATH_MAX sizeof(((struct sockaddr_un *)0)->sun_path)


ktp_session_t *ktp_session_new(const char *sun_path)
{
	ktp_session_t *session = NULL;

	assert(sun_path);
	if (!sun_path)
		return NULL;

	session = faux_zmalloc(sizeof(*session));
	assert(session);
	if (!session)
		return NULL;

	// Init
	session->state = KTP_SESSION_STATE_DISCONNECTED;
	session->sun_path = faux_str_dup(sun_path);
	assert(session->sun_path);
	session->net = faux_net_new();
	assert(session->net);

	return session;
}


void ktp_session_free(ktp_session_t *session)
{
	if (!session)
		return;

	ktp_session_disconnect(session);
	faux_net_free(session->net);
	faux_str_free(session->sun_path);
	faux_free(session);
}


int ktp_session_connect(ktp_session_t *session)
{
	int sock = -1;
	int opt = 1;
	struct sockaddr_un laddr = {};

	assert(session);
	if (!session)
		return -1;

	if (session->state != KTP_SESSION_STATE_DISCONNECTED)
		return faux_net_get_fd(session->net); // Already connected

	// Create socket
	if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
		return -1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
		close(sock);
		return -1;
	}
	laddr.sun_family = AF_UNIX;
	strncpy(laddr.sun_path, session->sun_path, USOCK_PATH_MAX);
	laddr.sun_path[USOCK_PATH_MAX - 1] = '\0';

	// Connect to server
	if (connect(sock, (struct sockaddr *)&laddr, sizeof(laddr))) {
		close(sock);
		return -1;
	}

	// Update session state
	faux_net_set_fd(session->net, sock);
	session->state = KTP_SESSION_STATE_IDLE;

	return sock;
}


int ktp_session_disconnect(ktp_session_t *session)
{
	int fd = -1;

	assert(session);
	if (!session)
		return -1;

	if (KTP_SESSION_STATE_DISCONNECTED == session->state)
		return -1; // Already disconnected
	session->state = KTP_SESSION_STATE_DISCONNECTED;
	fd = faux_net_get_fd(session->net);
	if (fd < 0)
		return fd; // Strange - already disconnected
	close(fd);
	faux_net_unset_fd(session->net);

	return fd;
}
