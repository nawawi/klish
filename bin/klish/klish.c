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

#include <klish/ktp.h>
#include <klish/ktp_session.h>

#include "private.h"


int main(int argc, char **argv)
{
	int retval = -1;
	struct options *opts = NULL;
	int unix_sock = -1;
	ktp_session_t *session = NULL;

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

	write(ktp_session_fd(session), "hello", 5);

	retval = 0;

err:
	ktp_session_free(session);
	ktp_disconnect(unix_sock);
	opts_free(opts);

	return retval;
}
