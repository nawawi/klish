/*
 * clish_config_callback.c
 *
 *
 * Callback hook to execute config operations.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <assert.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "private.h"
#include "konf/net.h"
#include "konf/buf.h"
#include "konf/query.h"
#include "clish/variable.h"

static int send_request(konf_client_t * client, char *command);
static int receive_answer(konf_client_t * client, konf_buf_t **data);
static int process_answer(konf_client_t * client, char *str, konf_buf_t *buf, konf_buf_t **data);
static int receive_data(konf_client_t * client, konf_buf_t *buf, konf_buf_t **data);

/*--------------------------------------------------------- */
bool_t
clish_config_callback(const clish_shell_t * shell,
		      const clish_command_t * cmd, clish_pargv_t * pargv)
{
	unsigned i;
	char *line;
	char *command = NULL;
	konf_client_t *client;
	konf_buf_t *buf = NULL;

	switch (clish_command__get_cfg_op(cmd)) {

	case CLISH_CONFIG_NONE:
		return BOOL_TRUE;

	case CLISH_CONFIG_SET:
		{
			char tmp[100];
			char *pattern;

			lub_string_cat(&command, "-s");
			pattern = clish_command__get_pattern(cmd, pargv);
			if (!pattern) {
				lub_string_free(command);
				return BOOL_FALSE;
			}

			line = clish_variable__get_line(cmd, pargv);
			lub_string_cat(&command, " -l \"");
			lub_string_cat(&command, line);
			lub_string_cat(&command, "\"");
			lub_string_free(line);

			lub_string_cat(&command, " -r \"");
			lub_string_cat(&command, pattern);
			lub_string_cat(&command, "\"");
			lub_string_free(pattern);

			if (clish_command__get_splitter(cmd) == BOOL_FALSE)
				lub_string_cat(&command, " -i");

			snprintf(tmp, sizeof(tmp) - 1, " -p 0x%x",
				 clish_command__get_priority(cmd));
			tmp[sizeof(tmp) - 1] = '\0';
			lub_string_cat(&command, tmp);

			for (i = 0; i < clish_command__get_depth(cmd); i++) {
				const char *str =
				    clish_shell__get_pwd(shell, i);
				if (!str)
					return BOOL_FALSE;
				lub_string_cat(&command, " \"");
				lub_string_cat(&command, str);
				lub_string_cat(&command, "\"");
			}
			break;
		}

	case CLISH_CONFIG_UNSET:
		{
			char tmp[100];
			char *pattern;

			lub_string_cat(&command, "-u");
			pattern = clish_command__get_pattern(cmd, pargv);
			if (!pattern) {
				lub_string_free(command);
				return BOOL_FALSE;
			}

			lub_string_cat(&command, " -r \"");
			lub_string_cat(&command, pattern);
			lub_string_cat(&command, "\"");
			lub_string_free(pattern);

			for (i = 0; i < clish_command__get_depth(cmd); i++) {
				const char *str =
				    clish_shell__get_pwd(shell, i);
				if (!str)
					return BOOL_FALSE;
				lub_string_cat(&command, " \"");
				lub_string_cat(&command, str);
				lub_string_cat(&command, "\"");
			}
			break;
		}

	case CLISH_CONFIG_DUMP:
		{
			char *file;

			lub_string_cat(&command, "-d");

			file = clish_command__get_file(cmd, pargv);
			if (file) {
				lub_string_cat(&command, " -f \"");
				if (file[0] != '\0')
					lub_string_cat(&command, file);
				else
					lub_string_cat(&command, "/tmp/running-config");
				lub_string_cat(&command, "\"");
			}
			break;
		}

	case CLISH_CONFIG_COPY:
		{
			char *file;
			lub_string_cat(&command, "-d -f ");
			file = clish_command__get_file(cmd, pargv);
			lub_string_cat(&command, file);
			lub_string_free(file);
			break;
		}

	default:
		return BOOL_FALSE;
	};

	client = clish_shell__get_client(shell);

#ifdef DEBUG
	fprintf(stderr, "CONFIG request: %s\n", command);
#endif

	if (send_request(client, command) < 0) {
		fprintf(stderr, "Cannot write to the running-config.\n");
	}

	if (receive_answer(client, &buf) < 0) {
		fprintf(stderr, "Cannot get answer from config daemon.\n");
	}

	lub_string_free(command);

	switch (clish_command__get_cfg_op(cmd)) {

	case CLISH_CONFIG_DUMP:
		{
			char *filename;

			filename = clish_command__get_file(cmd, pargv);
			if (filename) {
				FILE *fd;
				char str[1024];
				if (!(fd = fopen(filename, "r")))
					break;
				while (fgets(str, sizeof(str), fd))
					fprintf(clish_shell__get_ostream(shell),
						"%s", str);
				fclose(fd);
			}
			if (buf) {
				char *str;
				konf_buf_lseek(buf, 0);
				while ((str = konf_buf_preparse(buf))) {
					if (strlen(str) == 0) {
						lub_string_free(str);
						break;
					}
					fprintf(clish_shell__get_ostream(shell),
						"%s\n", str);
					lub_string_free(str);
				}
				konf_buf_delete(buf);
			}
			break;
		}

	default:
		break;
	};

	return BOOL_TRUE;
}

/*--------------------------------------------------------- */

static int send_request(konf_client_t * client, char *command)
{
	if ((konf_client_connect(client) < 0))
		return -1;

	if (konf_client_send(client, command) < 0) {
		if (konf_client_reconnect(client) < 0)
			return -1;
		if (konf_client_send(client, command) < 0)
			return -1;
	}

	return 0;
}

static int receive_answer(konf_client_t * client, konf_buf_t **data)
{
	konf_buf_t *buf;
	int nbytes;
	char *str;
	int retval = 0;
	int processed = 0;

	if ((konf_client_connect(client) < 0))
		return -1;

	buf = konf_buf_new(konf_client__get_sock(client));
	while ((!processed) && (nbytes = konf_buf_read(buf)) > 0) {
		while ((str = konf_buf_parse(buf))) {
			konf_buf_t *tmpdata = NULL;
			retval = process_answer(client, str, buf, &tmpdata);
			lub_string_free(str);
			if (retval < 0)
				return retval;
			if (retval == 0)
				processed = 1;
			if (tmpdata) {
				if (*data)
					konf_buf_delete(*data);
				*data = tmpdata;
			}
		}
	}
	konf_buf_delete(buf);

	return retval;
}

static int receive_data(konf_client_t * client, konf_buf_t *buf, konf_buf_t **data)
{
	konf_buf_t *tmpdata;
	char *str;
	int retval = 0;
	int processed = 0;

	if ((konf_client_connect(client) < 0))
		return -1;

	tmpdata = konf_buf_new(konf_client__get_sock(client));
	do {
		while ((str = konf_buf_parse(buf))) {
#ifdef DEBUG
			fprintf(stderr, "RECV DATA: [%s]\n", str);
#endif
			konf_buf_add(tmpdata, str, strlen(str) + 1);
			if (strlen(str) == 0) {
				processed = 1;
				lub_string_free(str);
				break;
			}
			lub_string_free(str);
		}
	} while ((!processed) && (konf_buf_read(buf)) > 0);
	if (!processed) {
		konf_buf_delete(tmpdata);
		*data = NULL;
		return -1;
	}

	*data = tmpdata;

	return 0;
}

static int process_answer(konf_client_t * client, char *str, konf_buf_t *buf, konf_buf_t **data)
{
	int res;
	konf_query_t *query;

	/* Parse query */
	query = konf_query_new();
	res = konf_query_parse_str(query, str);
	if (res < 0) {
		konf_query_free(query);
#ifdef DEBUG
		fprintf(stderr, "CONFIG error: Cannot parse answer string.\n");
#endif
		return -1;
	}

#ifdef DEBUG
	fprintf(stderr, "CONFIG answer: %s\n", str);
	// konf_query_dump(query);
#endif

	switch (konf_query__get_op(query)) {

	case konf_query_OP_OK:
		res = 0;
		break;

	case konf_query_OP_ERROR:
		res = -1;
		break;

	case konf_query_OP_STREAM:
		if (receive_data(client, buf, data) < 0)
			res = -1;
		else
			res = 1; /* wait for another answer */
		break;

	default:
		res = -1;
		break;
	}

	/* Free resources */
	konf_query_free(query);

	return res;
}
