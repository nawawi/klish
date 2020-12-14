#ifndef _klish_ktp_private_h
#define _klish_ktp_private_h

#include <faux/net.h>
#include <klish/ktp_session.h>

#define USOCK_PATH_MAX sizeof(((struct sockaddr_un *)0)->sun_path)


typedef enum {
	KTPD_SESSION_STATE_DISCONNECTED = 'd',
	KTPD_SESSION_STATE_IDLE = 'i',
	KTPD_SESSION_STATE_WAIT_FOR_PROCESS = 'p',
} ktpd_session_state_e;

struct ktpd_session_s {
	ktpd_session_state_e state;
	pid_t client_pid;
	faux_net_t *net;
};


typedef enum {
	KTP_SESSION_STATE_DISCONNECTED = 'd',
	KTP_SESSION_STATE_IDLE = 'i',
	KTP_SESSION_STATE_WAIT_FOR_COMPLETION = 'v',
	KTP_SESSION_STATE_WAIT_FOR_HELP = 'h',
	KTP_SESSION_STATE_WAIT_FOR_CMD = 'c',
} ktp_session_state_e;

struct ktp_session_s {
	ktp_session_state_e state;
	faux_net_t *net;
};

#endif // _klish_ktp_private_h
