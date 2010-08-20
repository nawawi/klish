/*
 * clish_config_callback.c
 *
 *
 * Callback hook to execute config operations.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <assert.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <sys/select.h>
#include <signal.h>

#include "clish/private.h"
#include "konf/tree.h"
#include "konf/query.h"
#include "konf/buf.h"
#include "lub/argv.h"
#include "lub/string.h"

#define KONFD_SOCKET_PATH "/tmp/konfd.socket"
#define KONFD_CONFIG_PATH "/tmp/running-config"

#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX 108
#endif
#define MAXMSG 1024

/* Global signal vars */
static volatile int sigterm = 0;
static void sighandler(int signo);

static char * process_query(int sock, konf_tree_t * conf, char *str);
int answer_send(int sock, char *command);
static int dump_running_config(int sock, konf_tree_t *conf, konf_query_t *query);

/*--------------------------------------------------------- */
int main(int argc, char **argv)
{
	int retval = 0;
	unsigned i;
	char *str;
	konf_tree_t *conf;
	lub_bintree_t bufs;
	konf_buf_t *tbuf;

	/* Network vars */
	int sock;
	struct sockaddr_un laddr;
	struct sockaddr_un raddr;
	fd_set active_fd_set, read_fd_set;

	/* Signal vars */
	struct sigaction sig_act;
	sigset_t sig_set;

	/* Set signal handler */
	sigemptyset(&sig_set);
	sigaddset(&sig_set, SIGTERM);
	sigaddset(&sig_set, SIGINT);
	sigaddset(&sig_set, SIGQUIT);
	
	sig_act.sa_flags = 0;
	sig_act.sa_mask = sig_set;
	sig_act.sa_handler = &sighandler;
	sigaction(SIGTERM, &sig_act, NULL);
	sigaction(SIGINT, &sig_act, NULL);
	sigaction(SIGQUIT, &sig_act, NULL);

	/* Configuration tree */
	conf = konf_tree_new("", 0);

	/* Initialize the tree of buffers */
	lub_bintree_init(&bufs,
		konf_buf_bt_offset(),
		konf_buf_bt_compare, konf_buf_bt_getkey);

	/* Create listen socket */
	if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "Cannot create socket: %s\n", strerror(errno));
/*		syslog(LOG_ERR, "Cannot create socket: %s\n", strerror(errno));
*/		return -1;
	}

	unlink(KONFD_SOCKET_PATH);
	laddr.sun_family = AF_UNIX;
	strncpy(laddr.sun_path, KONFD_SOCKET_PATH, UNIX_PATH_MAX);
	laddr.sun_path[UNIX_PATH_MAX - 1] = '\0';
	if (bind(sock, (struct sockaddr *)&laddr, sizeof(laddr))) {
		fprintf(stderr, "Can't bind()\n");
/*		syslog(LOG_ERR, "Can't bind()\n");
*/		return -1;
	}

	listen(sock, 5);

	/* Initialize the set of active sockets. */
	FD_ZERO(&active_fd_set);
	FD_SET(sock, &active_fd_set);

	/* Main loop */
	while (!sigterm) {
		int num;

		/* Block until input arrives on one or more active sockets. */
		read_fd_set = active_fd_set;
		num = select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL);
		if (num < 0) {
			if (EINTR == errno)
				continue;
			break;
		}
		if (0 == num)
			continue;

		/* Get string from stdout */
/*		if (!fgets(str, sizeof(str), stdin)) {
			enough = 1;
			continue;
		}
*/
		/* Service all the sockets with input pending. */
		for (i = 0; i < FD_SETSIZE; ++i) {
			if (FD_ISSET(i, &read_fd_set)) {
				if (i == sock) {
					/* Connection request on original socket. */
					int new;
					socklen_t size;
					
					size = sizeof(raddr);
					new = accept(sock, (struct sockaddr *)
						     &raddr, &size);
					if (new < 0) {
						fprintf(stderr, "accept");
						continue;
					}
					fprintf(stderr, "Server: connect %u\n", new);
					konf_buftree_remove(&bufs, new);
					tbuf = konf_buf_new(new);
					/* insert it into the binary tree for this conf */
					lub_bintree_insert(&bufs, tbuf);
					FD_SET(new, &active_fd_set);
				} else {
					int nbytes;
					/* Data arriving on an already-connected socket. */
					if ((nbytes = konf_buftree_read(&bufs, i)) <= 0) {
						close(i);
						FD_CLR(i, &active_fd_set);
						konf_buftree_remove(&bufs, i);
						continue;
					}
					while ((str = konf_buftree_parse(&bufs, i))) {
						char *answer;
						if (!(answer = process_query(i, conf, str)))
							answer = lub_string_dup("-e");
						lub_string_free(str);
						answer_send(i, answer);
						lub_string_free(answer);
					}
				}
			}
		}
	}

	/* Free resources */
	konf_tree_delete(conf);

	/* delete each buf */
	while ((tbuf = lub_bintree_findfirst(&bufs))) {
		/* remove the buf from the tree */
		lub_bintree_remove(&bufs, tbuf);
		/* release the instance */
		konf_buf_delete(tbuf);
	}

	return retval;
}

static char * process_query(int sock, konf_tree_t * conf, char *str)
{
	unsigned i;
	int res;
	konf_tree_t *iconf;
	konf_tree_t *tmpconf;
	konf_query_t *query;
	char *retval = NULL;
	konf_query_op_t ret;

	/* Parse query */
	query = konf_query_new();
	res = konf_query_parse_str(query, str);
	if (res < 0) {
		konf_query_free(query);
		return NULL;
	}
#ifdef DEBUG
	fprintf(stderr, "----------------------\n");
	fprintf(stderr, "REQUEST: %s\n", str);
/*	konf_query_dump(query);
*/
#endif

	/* Go through the pwd */
	iconf = conf;
	for (i = 0; i < konf_query__get_pwdc(query); i++) {
		if (!
		    (iconf =
		     konf_tree_find_conf(iconf, konf_query__get_pwd(query, i), 0))) {
			iconf = NULL;
			break;
		}
	}

	if (!iconf) {
		fprintf(stderr, "Unknown path\n");
		konf_query_free(query);
		return NULL;
	}

	switch (konf_query__get_op(query)) {

	case KONF_QUERY_OP_SET:
		if (konf_tree_find_conf(iconf, konf_query__get_line(query), 0)) {
			ret = KONF_QUERY_OP_OK;
			break;
		}
		konf_tree_del_pattern(iconf, konf_query__get_pattern(query));
		tmpconf = konf_tree_new_conf(iconf, 
			konf_query__get_line(query), konf_query__get_priority(query));
		if (!tmpconf) {
			ret = KONF_QUERY_OP_ERROR;
			break;
		}
		konf_tree__set_splitter(tmpconf, konf_query__get_splitter(query));
		ret = KONF_QUERY_OP_OK;
		break;

	case KONF_QUERY_OP_UNSET:
		konf_tree_del_pattern(iconf, konf_query__get_pattern(query));
		ret = KONF_QUERY_OP_OK;
		break;

	case KONF_QUERY_OP_DUMP:
		if (dump_running_config(sock, iconf, query))
			ret = KONF_QUERY_OP_ERROR;
		else
			ret = KONF_QUERY_OP_OK;
		break;

	default:
		ret = KONF_QUERY_OP_ERROR;
		break;
	}

#ifdef DEBUG
	/* Print whole tree */
	konf_tree_fprintf(conf, stderr, NULL, -1, 0);
#endif

	/* Free resources */
	konf_query_free(query);

	switch (ret) {
	case KONF_QUERY_OP_OK:
		lub_string_cat(&retval, "-o");
		break;
	case KONF_QUERY_OP_ERROR:
		lub_string_cat(&retval, "-e");
		break;
	default:
		lub_string_cat(&retval, "-e");
		break;
	};

	return retval;
}

/*
 * Signal handler for temination signals (like SIGTERM, SIGINT, ...)
 */
static void sighandler(int signo)
{
	sigterm = 1;
}

int answer_send(int sock, char *command)
{
	return send(sock, command, strlen(command) + 1, MSG_NOSIGNAL);
}

static int dump_running_config(int sock, konf_tree_t *conf, konf_query_t *query)
{
	FILE *fd;
	char *filename;
	int dupsock = -1;

	if ((filename = konf_query__get_path(query))) {
		if (!(fd = fopen(filename, "w")))
			return -1;
	} else {
		if ((dupsock = dup(sock)) < 0)
			return -1;
		fd = fdopen(dupsock, "w");
	}
	if (!filename) {
		fprintf(fd, "-t\n");
#ifdef DEBUG
		fprintf(stderr, "ANSWER: -t\n");
#endif
	}
	konf_tree_fprintf(conf, fd, konf_query__get_pattern(query),
		konf_query__get_pwdc(query) - 1, 0);
	if (!filename) {
		fprintf(fd, "\n");
#ifdef DEBUG
		fprintf(stderr, "SEND DATA: \n");
#endif
	}

	fclose(fd);

	return 0;
}


