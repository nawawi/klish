#ifndef _klish_ktp_session_h
#define _klish_ktp_session_h

#include <faux/faux.h>
#include <faux/list.h>
#include <klish/ksession.h>

#define USOCK_PATH_MAX sizeof(((struct sockaddr_un *)0)->sun_path)

#define KLISH_DEFAULT_UNIX_SOCKET_PATH "/tmp/klish-unix-socket"

typedef struct ktpd_session_s ktpd_session_t;
typedef struct ktp_session_s ktp_session_t;
//typedef struct ktpd_clients_s ktpd_clients_t;

typedef bool_t (*faux_session_stall_cb_fn)(ktpd_session_t *session,
	void *user_data);

C_DECL_BEGIN

// Client KTP session
ktp_session_t *ktp_session_new(int sock);
void ktp_session_free(ktp_session_t *session);
bool_t ktp_session_connected(ktp_session_t *session);
int ktp_session_fd(const ktp_session_t *session);

// Server KTP session
ktpd_session_t *ktpd_session_new(int sock, const kscheme_t *scheme,
	const char *start_entry);
void ktpd_session_free(ktpd_session_t *session);
bool_t ktpd_session_connected(ktpd_session_t *session);
int ktpd_session_fd(const ktpd_session_t *session);
bool_t ktpd_session_async_in(ktpd_session_t *session);
bool_t ktpd_session_async_out(ktpd_session_t *session);
void ktpd_session_set_stall_cb(ktpd_session_t *session,
	faux_session_stall_cb_fn stall_cb, void *user_data);

/*
// Server's KTP clients database
ktpd_clients_t *ktpd_clients_new(void);
void ktpd_clients_free(ktpd_clients_t *db);
ktpd_session_t *ktpd_clients_find(const ktpd_clients_t *db, int fd);
ktpd_session_t *ktpd_clients_add(ktpd_clients_t *db, int fd);
int ktpd_clients_del(ktpd_clients_t *db, int fd);
faux_list_node_t *ktpd_clients_init_iter(const ktpd_clients_t *db);
ktpd_session_t *ktpd_clients_each(faux_list_node_t **iter);
void ktpd_clients_debug(ktpd_clients_t *db);
*/

C_DECL_END

#endif // _klish_ktp_session_h
