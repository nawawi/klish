#ifndef _klish_ktp_session_h
#define _klish_ktp_session_h

#include <faux/faux.h>
#include <faux/list.h>
#include <faux/eloop.h>
#include <faux/error.h>
#include <klish/ksession.h>
#include <klish/ktp.h>

#define USOCK_PATH_MAX sizeof(((struct sockaddr_un *)0)->sun_path)

#define KLISH_DEFAULT_UNIX_SOCKET_PATH "/tmp/klish-unix-socket"

typedef struct ktpd_session_s ktpd_session_t;
typedef struct ktp_session_s ktp_session_t;


C_DECL_BEGIN

// Common KTP functions
int ktp_connect_unix(const char *sun_path);
void ktp_disconnect(int fd);
int ktp_accept(int listen_sock);

bool_t ktp_check_header(faux_hdr_t *hdr);
faux_msg_t *ktp_msg_preform(ktp_cmd_e cmd, uint32_t status);
bool_t ktp_send_error(faux_async_t *async, ktp_cmd_e cmd, const char *error);

bool_t ktp_peer_ev(faux_eloop_t *eloop, faux_eloop_type_e type,
	void *associated_data, void *user_data);
bool_t ktp_stall_cb(faux_async_t *async, size_t len, void *user_data);

// Help structure
typedef struct help_s {
	char *prefix;
	char *line;
} help_t;

int help_compare(const void *first, const void *second);
int help_kcompare(const void *key, const void *list_item);
help_t *help_new(char *prefix, char *line);
void help_free(void *ptr);


// Client KTP session

typedef enum {
	KTP_SESSION_STATE_ERROR = 'e', // Some unknown error
	KTP_SESSION_STATE_DISCONNECTED = 'd',
	KTP_SESSION_STATE_UNAUTHORIZED = 'a',
	KTP_SESSION_STATE_IDLE = 'i',
	KTP_SESSION_STATE_WAIT_FOR_COMPLETION = 'v',
	KTP_SESSION_STATE_WAIT_FOR_HELP = 'h',
	KTP_SESSION_STATE_WAIT_FOR_CMD = 'c',
} ktp_session_state_e;

typedef enum {
	KTP_SESSION_CB_STDIN,
	KTP_SESSION_CB_STDOUT,
	KTP_SESSION_CB_STDERR,
	KTP_SESSION_CB_AUTH_ACK,
	KTP_SESSION_CB_CMD_ACK_INCOMPLETED,
	KTP_SESSION_CB_CMD_ACK,
	KTP_SESSION_CB_COMPLETION_ACK,
	KTP_SESSION_CB_HELP_ACK,
	KTP_SESSION_CB_EXIT,
	KTP_SESSION_CB_NOTIFICATION,
	KTP_SESSION_CB_MAX,
} ktp_session_cb_e;

typedef bool_t (*ktp_session_stdin_cb_fn)(ktp_session_t *ktp,
	size_t len, void *udata);
typedef bool_t (*ktp_session_stdout_cb_fn)(ktp_session_t *ktp,
	const char *line, size_t len, void *udata);
typedef bool_t (*ktp_session_event_cb_fn)(ktp_session_t *ktp,
	const faux_msg_t *msg, void *udata);

ktp_session_t *ktp_session_new(int sock, faux_eloop_t *eloop);
void ktp_session_free(ktp_session_t *session);

faux_eloop_t *ktp_session_eloop(const ktp_session_t *ktp);
bool_t ktp_session_done(const ktp_session_t *ktp);
bool_t ktp_session_set_done(ktp_session_t *ktp, bool_t done);
faux_error_t *ktp_session_error(const ktp_session_t *ktp);
bool_t ktp_session_set_cb(ktp_session_t *ktp, ktp_session_cb_e cb_id,
	void *fn, void *udata);
bool_t ktp_session_connected(ktp_session_t *session);
int ktp_session_fd(const ktp_session_t *session);
faux_async_t *ktp_session_async(const ktp_session_t *ktp);
bool_t ktp_session_stop_on_answer(const ktp_session_t *ktp);
bool_t ktp_session_set_stop_on_answer(ktp_session_t *ktp, bool_t stop_on_answer);
ktp_session_state_e ktp_session_state(const ktp_session_t *ktp);

bool_t ktp_session_cmd(ktp_session_t *ktp, const char *line,
	faux_error_t *error, bool_t dry_run);
bool_t ktp_session_auth(ktp_session_t *ktp, faux_error_t *error);
bool_t ktp_session_completion(ktp_session_t *ktp, const char *line,
	bool_t dry_run);
bool_t ktp_session_help(ktp_session_t *ktp, const char *line);
bool_t ktp_session_stdin(ktp_session_t *ktp, const char *line, size_t line_len);
bool_t ktp_session_stdin_close(ktp_session_t *ktp);
bool_t ktp_session_stdout_close(ktp_session_t *ktp);
bool_t ktp_session_stderr_close(ktp_session_t *ktp);
bool_t ktp_session_retcode(ktp_session_t *ktp, int *retcode);
ktp_status_e ktp_session_cmd_features(const ktp_session_t *ktp);
bool_t ktp_session_stdout_need_newline(ktp_session_t *ktp);
bool_t ktp_session_stderr_need_newline(ktp_session_t *ktp);
int ktp_session_last_stream(ktp_session_t *ktp);


// Server KTP session
typedef bool_t (*ktpd_session_stall_cb_fn)(ktpd_session_t *session,
	void *user_data);

ktpd_session_t *ktpd_session_new(int sock, kscheme_t *scheme,
	const char *start_entry, faux_eloop_t *eloop);
void ktpd_session_free(ktpd_session_t *session);
bool_t ktpd_session_connected(ktpd_session_t *session);
int ktpd_session_fd(const ktpd_session_t *session);
bool_t ktpd_session_async_in(ktpd_session_t *session);
bool_t ktpd_session_async_out(ktpd_session_t *session);

C_DECL_END

#endif // _klish_ktp_session_h
