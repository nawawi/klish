#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <assert.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/un.h>

#include "clish/private.h"
#include "lub/string.h"
#include "private.h"

#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX 108
#endif

konf_client_t *konf_client_new(const char *path)
{
	konf_client_t *client;

	if (!path)
		return NULL;

	if (!(client = malloc(sizeof(*client))))
		return NULL;

	client->sock = -1; /* socket is not created yet */
	client->path = lub_string_dup(path);

	return client;
}

void konf_client_free(konf_client_t *client)
{
	if (!client)
		return;
	if (client->sock != -1)
		konf_client_disconnect(client);
	lub_string_free(client->path);

	free(client);
}

int konf_client_connect(konf_client_t *client)
{
	struct sockaddr_un raddr;

	if (client->sock >= 0)
		return client->sock;

	if ((client->sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
		return client->sock;

	raddr.sun_family = AF_UNIX;
	strncpy(raddr.sun_path, client->path, UNIX_PATH_MAX);
	raddr.sun_path[UNIX_PATH_MAX - 1] = '\0';
	if (connect(client->sock, (struct sockaddr *)&raddr, sizeof(raddr))) {
		close(client->sock);
		client->sock = -1;
	}

	return client->sock;
}

void konf_client_disconnect(konf_client_t *client)
{
	if (client->sock >= 0) {
		close(client->sock);
		client->sock = -1;
	}
}

int konf_client_reconnect(konf_client_t *client)
{
	konf_client_disconnect(client);
	return konf_client_connect(client);
}

int konf_client_send(konf_client_t *client, char *command)
{
	if (client->sock < 0)
		return client->sock;

	return send(client->sock, command, strlen(command) + 1, MSG_NOSIGNAL);
}

int konf_client__get_sock(konf_client_t *client)
{
	return client->sock;
}
