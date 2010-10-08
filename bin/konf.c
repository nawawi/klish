/*
 * konf.c
 *
 *
 * The client to communicate to konfd configuration daemon.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "konf/net.h"
#include "konf/tree.h"
#include "konf/query.h"
#include "konf/buf.h"
#include "lub/argv.h"
#include "lub/string.h"

#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX 108
#endif
#define MAXMSG 1024

static int receive_answer(konf_client_t * client, konf_buf_t **data);
static int receive_data(konf_client_t * client, konf_buf_t *buf, konf_buf_t **data);
static int process_answer(konf_client_t * client, char *str, konf_buf_t *buf, konf_buf_t **data);

/*--------------------------------------------------------- */
int main(int argc, char **argv)
{
	int res = -1;
	unsigned i;
	konf_client_t *client = NULL;
	konf_buf_t *buf = NULL;
	char *line = NULL;
	char *str = NULL;
	const char *socket_path = KONFD_SOCKET_PATH;

	/* Get request line from the args */
	for (i = 1; i < argc; i++) {
		char *space = NULL;
		if (NULL != line)
			lub_string_cat(&line, " ");
		space = strchr(argv[i], ' ');
		if (space)
			lub_string_cat(&line, "\"");
		lub_string_cat(&line, argv[i]);
		if (space)
			lub_string_cat(&line, "\"");
	}
	if (!line) {
		fprintf(stderr, "Not enough arguments.\n");
		goto err;
	}
#ifdef DEBUG
	printf("REQUEST: %s\n", line);
#endif

	if (!(client = konf_client_new(socket_path))) {
		fprintf(stderr, "Can't create internal data structures.\n");
		goto err;
	}

	if (konf_client_connect(client) < 0) {
		fprintf(stderr, "Can't connect to %s socket.\n", socket_path);
		goto err;
	}

	if (konf_client_send(client, line) < 0) {
		fprintf(stderr, "Can't connect to %s socket.\n", socket_path);
		goto err;
	}

	if (receive_answer(client, &buf) < 0) {
		fprintf(stderr, "Can't get answer from the config daemon.\n");
	}

	if (buf) {
		konf_buf_lseek(buf, 0);
		while ((str = konf_buf_preparse(buf))) {
			if (strlen(str) == 0) {
				lub_string_free(str);
				break;
			}
			fprintf(stdout, "%s\n", str);
			lub_string_free(str);
		}
		konf_buf_delete(buf);
	}

	res = 0;
err:
	lub_string_free(line);
	konf_client_free(client);

	return res;
}

/*--------------------------------------------------------- */
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

/*--------------------------------------------------------- */
static int receive_data(konf_client_t * client, konf_buf_t *buf, konf_buf_t **data)
{
	konf_buf_t *tmpdata;
	char *str;
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

/*--------------------------------------------------------- */
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
	fprintf(stderr, "ANSWER: %s\n", str);
/*	konf_query_dump(query);
*/
#endif

	switch (konf_query__get_op(query)) {

	case KONF_QUERY_OP_OK:
		res = 0;
		break;

	case KONF_QUERY_OP_ERROR:
		res = -1;
		break;

	case KONF_QUERY_OP_STREAM:
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
