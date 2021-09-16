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


static bool_t stdout_cb(ktp_session_t *ktp, const char *line, size_t len,
	void *user_data);
static bool_t stderr_cb(ktp_session_t *ktp, const char *line, size_t len,
	void *user_data);


int main(int argc, char **argv)
{
	int retval = -1;
	struct options *opts = NULL;
	int unix_sock = -1;
	ktp_session_t *ktp = NULL;

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
	ktp = ktp_session_new(unix_sock);
	assert(ktp);
	if (!ktp) {
		fprintf(stderr, "Error: Can't create klish session\n");
		goto err;
	}
	ktp_session_set_stdout_cb(ktp, stdout_cb, NULL);
	ktp_session_set_stderr_cb(ktp, stderr_cb, NULL);

	ktp_session_req_cmd(ktp, opts->line, NULL);

	retval = 0;
err:
	ktp_session_free(ktp);
	ktp_disconnect(unix_sock);
	opts_free(opts);

	return retval;
}


static bool_t stdout_cb(ktp_session_t *ktp, const char *line, size_t len,
	void *user_data)
{
	if (write(STDOUT_FILENO, line, len) < 0)
		return BOOL_FALSE;

	ktp = ktp;
	user_data = user_data;

	return BOOL_TRUE;
}


static bool_t stderr_cb(ktp_session_t *ktp, const char *line, size_t len,
	void *user_data)
{
	if (write(STDERR_FILENO, line, len) < 0)
		return BOOL_FALSE;

	ktp = ktp;
	user_data = user_data;

	return BOOL_TRUE;
}
