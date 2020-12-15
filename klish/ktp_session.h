#ifndef _klish_ktp_session_h
#define _klish_ktp_session_h

#define USOCK_PATH_MAX sizeof(((struct sockaddr_un *)0)->sun_path)

#define KLISH_DEFAULT_UNIX_SOCKET_PATH "/tmp/klish-unix-socket"

typedef struct ktpd_session_s ktpd_session_t;
typedef struct ktp_session_s ktp_session_t;

C_DECL_BEGIN

// Client KTP session
ktp_session_t *ktp_session_new(int sock);
void ktp_session_free(ktp_session_t *session);
bool_t ktp_session_connected(ktp_session_t *session);
int ktp_session_get_socket(ktp_session_t *session);

// Server KTP session
ktpd_session_t *ktpd_session_new(int sock);
void ktpd_session_free(ktpd_session_t *session);
bool_t ktpd_session_connected(ktpd_session_t *session);
int ktpd_session_get_socket(ktpd_session_t *session);

C_DECL_END

#endif // _klish_ktp_session_h
