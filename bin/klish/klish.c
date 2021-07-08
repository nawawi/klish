#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <faux/faux.h>
#include <faux/str.h>
#include <faux/msg.h>

#include <klish/ktp.h>
#include <klish/ktp_session.h>

#include "private.h"


int main(int argc, char **argv)
{
	int retval = -1;
	struct options *opts = NULL;
	int unix_sock = -1;
	ktp_session_t *session = NULL;
	faux_msg_t *msg = NULL;
	faux_net_t *net = NULL;

	// Parse command line options
	opts = opts_init();
	if (opts_parse(argc, argv, opts)) {
		fprintf(stderr, "Error: Can't parse command line options\n");
		goto err;
	}

	// Connect to server
	unix_sock = ktp_connect_unix(opts->unix_socket_path);
	if (unix_sock < 0) {
		fprintf(stderr, "Error: Can't connect to server\n");
		goto err;
	}
	session = ktp_session_new(unix_sock);
	assert(session);
	if (!session) {
		fprintf(stderr, "Error: Can't create klish session\n");
		goto err;
	}

	net = faux_net_new();
	faux_net_set_fd(net, ktp_session_fd(session));
	msg = faux_msg_new(KTP_MAGIC, KTP_MAJOR, KTP_MINOR);
	faux_msg_set_cmd(msg, KTP_CMD);
	if (opts->line)
		faux_msg_add_param(msg, KTP_PARAM_LINE,
			opts->line, strlen(opts->line));
	faux_msg_debug(msg);
	faux_msg_send(msg, net);
	faux_msg_free(msg);

	msg = faux_msg_recv(net);
	faux_msg_debug(msg);
	if (KTP_STATUS_IS_ERROR(faux_msg_get_status(msg))) {
		char *error = faux_msg_get_str_param_by_type(msg, KTP_PARAM_ERROR);
		if (error) {
			printf("Error: %s\n", error);
			faux_str_free(error);
		}
	}
	faux_msg_free(msg);

	faux_net_free(net);

	retval = 0;

err:
	ktp_session_free(session);
	ktp_disconnect(unix_sock);
	opts_free(opts);

	return retval;
}
