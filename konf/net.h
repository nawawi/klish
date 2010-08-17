#ifndef _konf_net_h
#define _konf_net_h

typedef struct konf_client_s konf_client_t;

#define KONFD_SOCKET_PATH "/tmp/konfd.socket"

konf_client_t *konf_client_new(char *path);
void konf_client_free(konf_client_t *client);
int konf_client_connect(konf_client_t *client);
void konf_client_disconnect(konf_client_t *client);
int konf_client_reconnect(konf_client_t *client);
int konf_client_send(konf_client_t *client, char *command);
int konf_client__get_sock(konf_client_t *client);

#endif
