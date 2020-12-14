#ifndef _klish_ktp_session_h
#define _klish_ktp_session_h


typedef struct ktpd_session_s ktpd_session_t;
typedef struct ktp_session_s ktp_session_t;

C_DECL_BEGIN

// Client KTP session
ktp_session_t *ktp_session_new(const char *sun_path);
void ktp_session_free(ktp_session_t *session);
int ktp_session_connect(ktp_session_t *session);
int ktp_session_disconnect(ktp_session_t *session);

C_DECL_END

#endif // _klish_ktp_session_h
