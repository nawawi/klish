#ifndef _net_h
#define _net_h

typedef struct conf_client_s conf_client_t;

#include "cliconf/net/private.h"

#define CONFD_SOCKET_PATH "/tmp/confd.socket"

conf_client_t *conf_client_new(char *path);
void conf_client_free(conf_client_t *client);
int conf_client_connect(conf_client_t *client);
void conf_client_disconnect(conf_client_t *client);
int conf_client_reconnect(conf_client_t *client);
int conf_client_send(conf_client_t *client, char *command);
int conf_client__get_sock(conf_client_t *client);

#endif
