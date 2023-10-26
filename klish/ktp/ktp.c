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
#include <faux/msg.h>
#include <faux/eloop.h>
#include <faux/async.h>
#include <klish/ktp_session.h>


int ktp_connect_unix(const char *sun_path)
{
	int sock = -1;
	int opt = 1;
	struct sockaddr_un laddr = {};

	assert(sun_path);
	if (!sun_path)
		return -1;

	// Create socket
	if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
		return -1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
		close(sock);
		return -1;
	}
	laddr.sun_family = AF_UNIX;
	strncpy(laddr.sun_path, sun_path, USOCK_PATH_MAX);
	laddr.sun_path[USOCK_PATH_MAX - 1] = '\0';

	// Connect to server
	if (connect(sock, (struct sockaddr *)&laddr, sizeof(laddr))) {
		close(sock);
		return -1;
	}

	return sock;
}


void ktp_disconnect(int fd)
{
	if (fd < 0)
		return;
	close(fd);
}


int ktp_accept(int listen_sock)
{
	int new_conn = -1;

	new_conn = accept(listen_sock, NULL, NULL);

	return new_conn;
}


bool_t ktp_check_header(faux_hdr_t *hdr)
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


faux_msg_t *ktp_msg_preform(ktp_cmd_e cmd, uint32_t status)
{
	faux_msg_t *msg = NULL;

	msg = faux_msg_new(KTP_MAGIC, KTP_MAJOR, KTP_MINOR);
	assert(msg);
	if (!msg)
		return NULL;
	faux_msg_set_cmd(msg, cmd);
	faux_msg_set_status(msg, status);

	return msg;
}


bool_t ktp_send_error(faux_async_t *async, ktp_cmd_e cmd, const char *error)
{
	faux_msg_t *msg = NULL;

	assert(async);
	if (!async)
		return BOOL_FALSE;

	msg = ktp_msg_preform(cmd, KTP_STATUS_ERROR);
	if (error)
		faux_msg_add_param(msg, KTP_PARAM_ERROR, error, strlen(error));
	faux_msg_send_async(msg, async);
	faux_msg_free(msg);

	return BOOL_TRUE;
}


bool_t ktp_peer_ev(faux_eloop_t *eloop, faux_eloop_type_e type,
	void *associated_data, void *user_data)
{
	faux_eloop_info_fd_t *info = (faux_eloop_info_fd_t *)associated_data;
	faux_async_t *async = (faux_async_t *)user_data;

	assert(async);

	// Write data
	if (info->revents & POLLOUT) {
		faux_eloop_exclude_fd_event(eloop, info->fd, POLLOUT);
		if (faux_async_out_easy(async) < 0) {
			// Someting went wrong
			faux_eloop_del_fd(eloop, info->fd);
			syslog(LOG_ERR, "Problem with async output");
			return BOOL_FALSE; // Stop event loop
		}
	}

	// Read data
	if (info->revents & POLLIN) {
		if (faux_async_in_easy(async) < 0) {
			// Someting went wrong
			faux_eloop_del_fd(eloop, info->fd);
			syslog(LOG_ERR, "Problem with async input");
			return BOOL_FALSE; // Stop event loop
		}
	}

	// EOF
	if (info->revents & POLLHUP) {
		faux_eloop_del_fd(eloop, info->fd);
		syslog(LOG_DEBUG, "Close connection %d", info->fd);
		return BOOL_FALSE; // Stop event loop
	}

	// POLLERR
	if (info->revents & POLLERR) {
		faux_eloop_del_fd(eloop, info->fd);
		syslog(LOG_DEBUG, "POLLERR received %d", info->fd);
		return BOOL_FALSE; // Stop event loop
	}

	// POLLNVAL
	if (info->revents & POLLNVAL) {
		faux_eloop_del_fd(eloop, info->fd);
		syslog(LOG_DEBUG, "POLLNVAL received %d", info->fd);
		return BOOL_FALSE; // Stop event loop
	}

	type = type; // Happy compiler

	return BOOL_TRUE;
}


bool_t ktp_stall_cb(faux_async_t *async, size_t len, void *user_data)
{
	faux_eloop_t *eloop = (faux_eloop_t *)user_data;

	assert(eloop);

	faux_eloop_include_fd_event(eloop, faux_async_fd(async), POLLOUT);

	len = len; // Happy compiler

	return BOOL_TRUE;
}
