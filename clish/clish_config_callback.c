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
#include <limits.h>

#include "private.h"
#include "konf/net.h"
#include "konf/buf.h"
#include "konf/query.h"
#include "clish/variable.h"

static int send_request(konf_client_t * client, char *command);
static int receive_answer(konf_client_t * client, konf_buf_t **data);
static int process_answer(konf_client_t * client, char *str, konf_buf_t *buf, konf_buf_t **data);

/*--------------------------------------------------------- */
bool_t
clish_config_callback(const clish_shell_t * this,
	const clish_command_t * cmd, clish_pargv_t * pargv)
{
	unsigned i;
	char *command = NULL;
	konf_client_t *client;
	konf_buf_t *buf = NULL;
	const char *viewid = NULL;
	char *str = NULL;
	char tmp[PATH_MAX + 100];
	clish_config_operation_t op;

	if (!this)
		return BOOL_TRUE;

	viewid = clish_shell__get_viewid(this);
	op = clish_command__get_cfg_op(cmd);

	switch (op) {

	case CLISH_CONFIG_NONE:
		return BOOL_TRUE;

	case CLISH_CONFIG_SET:
		/* Add set operation */
		lub_string_cat(&command, "-s");

		/* Add entered line */
		str = clish_variable__get_line(cmd, pargv);
		lub_string_cat(&command, " -l \"");
		lub_string_cat(&command, str);
		lub_string_cat(&command, "\"");
		lub_string_free(str);

		/* Add splitter */
		if (clish_command__get_splitter(cmd) == BOOL_FALSE)
			lub_string_cat(&command, " -i");

		/* Add unique */
		if (clish_command__get_unique(cmd) == BOOL_FALSE)
			lub_string_cat(&command, " -n");

		break;

	case CLISH_CONFIG_UNSET:
		/* Add unset operation */
		lub_string_cat(&command, "-u");
		break;

	case CLISH_CONFIG_DUMP:
		/* Add dump operation */
		lub_string_cat(&command, "-d");

		/* Add filename */
		str = clish_command__get_file(cmd, viewid, pargv);
		if (str) {
			lub_string_cat(&command, " -f \"");
			if (str[0] != '\0')
				lub_string_cat(&command, str);
			else
				lub_string_cat(&command, "/tmp/running-config");
			lub_string_cat(&command, "\"");
			lub_string_free(str);
		}
		break;

	default:
		return BOOL_FALSE;
	};

	/* Add pattern */
	if ((CLISH_CONFIG_SET == op) || (CLISH_CONFIG_UNSET == op)) {
		str = clish_command__get_pattern(cmd, viewid, pargv);
		if (!str) {
			lub_string_free(command);
			return BOOL_FALSE;
		}
		lub_string_cat(&command, " -r \"");
		lub_string_cat(&command, str);
		lub_string_cat(&command, "\"");
		lub_string_free(str);
	}

	/* Add priority */
	if (clish_command__get_priority(cmd) != 0) {
		snprintf(tmp, sizeof(tmp) - 1, " -p 0x%x",
			clish_command__get_priority(cmd));
		tmp[sizeof(tmp) - 1] = '\0';
		lub_string_cat(&command, tmp);
	}

	/* Add sequence */
	if (clish_command__is_seq(cmd)) {
		snprintf(tmp, sizeof(tmp) - 1, " -q %u",
			clish_command__get_seq(cmd,
			viewid, pargv));
		tmp[sizeof(tmp) - 1] = '\0';
		lub_string_cat(&command, tmp);
	}

	/* Add pwd */
	str = clish_shell__get_pwd_full(this,
		clish_command__get_cfg_depth(cmd, viewid, pargv));
	if (str) {
		lub_string_cat(&command, " ");
		lub_string_cat(&command, str);
		lub_string_free(str);
	}

	client = clish_shell__get_client(this);
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

	/* Postprocessing. Get data from daemon etc. */
	switch (op) {

	case CLISH_CONFIG_DUMP:
		if (buf) {
			konf_buf_lseek(buf, 0);
			while ((str = konf_buf_preparse(buf))) {
				if (strlen(str) == 0) {
					lub_string_free(str);
					break;
				}
				fprintf(clish_shell__get_ostream(this),
					"%s\n", str);
				lub_string_free(str);
			}
			konf_buf_delete(buf);
		}
		break;

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
			if (retval < 0) {
				konf_buf_delete(buf);
				return retval;
			}
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

	case KONF_QUERY_OP_OK:
		res = 0;
		break;

	case KONF_QUERY_OP_ERROR:
		res = -1;
		break;

	case KONF_QUERY_OP_STREAM:
		if (!(*data = konf_client_recv_data(client, buf)))
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
