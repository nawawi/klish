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
#include <klish/ktp.h>

#include "private.h"


int ktp_connect(const char *sun_path)
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
