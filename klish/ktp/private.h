#ifndef _klish_ktp_private_h
#define _klish_ktp_private_h

#include <faux/net.h>
#include <faux/async.h>
#include <faux/msg.h>
#include <klish/ktp_session.h>


typedef enum {
	KTPD_SESSION_STATE_DISCONNECTED = 'd',
	KTPD_SESSION_STATE_NOT_AUTHORIZED = 'a',
	KTPD_SESSION_STATE_IDLE = 'i',
	KTPD_SESSION_STATE_WAIT_FOR_PROCESS = 'p',
} ktpd_session_state_e;

struct ktpd_session_s {
	ktpd_session_state_e state;
	uid_t uid;
	gid_t gid;
	char *user;
	faux_async_t *async;
	faux_session_stall_cb_f stall_cb; // Stall callback
	void *stall_udata;
	faux_hdr_t *hdr; // Engine will receive header and then msg
};


typedef enum {
	KTP_SESSION_STATE_DISCONNECTED = 'd',
	KTP_SESSION_STATE_NOT_AUTHORIZED = 'a',
	KTP_SESSION_STATE_IDLE = 'i',
	KTP_SESSION_STATE_WAIT_FOR_COMPLETION = 'v',
	KTP_SESSION_STATE_WAIT_FOR_HELP = 'h',
	KTP_SESSION_STATE_WAIT_FOR_CMD = 'c',
} ktp_session_state_e;

struct ktp_session_s {
	ktp_session_state_e state;
	faux_net_t *net;
};


struct ktpd_clients_s {
	faux_list_t *list;
};

#endif // _klish_ktp_private_h
