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
#include <faux/list.h>
#include <faux/file.h>

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

	// Commands from cmdline
	if (faux_list_len(opts->commands) > 0) {
		const char *line = NULL;
		faux_list_node_t *iter = faux_list_head(opts->commands);
		while ((line = faux_list_each(&iter))) {
			faux_error_t *error = faux_error_new();
			int retcode = -1;
			bool_t rc = BOOL_FALSE;
			if (!opts->quiet)
				fprintf(stderr, "%s\n", line);
			rc = ktp_session_req_cmd(ktp, line, &retcode, error);
			if (!rc || (faux_error_len(error) > 0)) {
				fprintf(stderr, "Error:\n");
				faux_error_fshow(error, stderr);
			}
			if (rc)
				fprintf(stderr, "Retcode: %d\n", retcode);
			faux_error_free(error);
		}

	// Commands from files
	} else if (faux_list_len(opts->files) > 0) {
		const char *filename = NULL;
		faux_list_node_t *iter =  faux_list_head(opts->files);
		while ((filename = (const char *)faux_list_each(&iter))) {
			char *line = NULL;
			faux_file_t *fd = NULL;
			fd = faux_file_open(filename, O_RDONLY, 0);
			while ((line = faux_file_getline(fd))) {
				faux_error_t *error = faux_error_new();
				int retcode = -1;
				bool_t rc = BOOL_FALSE;
				if (!opts->quiet)
					fprintf(stderr, "%s\n", line);
				rc = ktp_session_req_cmd(ktp, line, &retcode, error);
				if (!rc || (faux_error_len(error) > 0)) {
					fprintf(stderr, "Error:\n");
					faux_error_fshow(error, stderr);
				}
				if (rc)
					fprintf(stderr, "Retcode: %d\n", retcode);
				faux_error_free(error);
				faux_str_free(line);
			}
			faux_file_close(fd);
		}

	// Interactive shell
	} else {
	
	
	}

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
