#define _GNU_SOURCE
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <syslog.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/fsuid.h>
#include <sys/wait.h>
#include <poll.h>
#include <time.h>

#include <faux/faux.h>
#include <faux/str.h>
#include <faux/ini.h>
#include <faux/log.h>
#include <faux/sched.h>
#include <faux/sysdb.h>
#include <faux/net.h>
#include <faux/list.h>
#include <faux/conv.h>
#include <faux/file.h>
#include <faux/eloop.h>
#include <faux/error.h>

#include <klish/ktp.h>
#include <klish/ktp_session.h>

#include <klish/kscheme.h>

#include "private.h"

ischeme_t sch = {

  PTYPE_LIST

    PTYPE {
      .name = "ptype1",
    },

    PTYPE {
      .name = "ptype2",
    },

  END_PTYPE_LIST,

  VIEW_LIST

    VIEW {
      .name = "view1",
      COMMAND_LIST

        COMMAND {
          .name = "command1",
          .help = "help1",
        },

        COMMAND {
          .name = "command2",
          .help = "help1",
        },

        COMMAND {
          .name = "command3",
          .help = "help1",
        },

      END_COMMAND_LIST,
    },

    VIEW {
      .name = "view2",
    },

    VIEW {
      .name = "view1",
      COMMAND_LIST

        COMMAND {
          .name = "command4",
          .help = "help1",
        },

        COMMAND {
          .name = "command5",
          .help = "help1",
        },

      END_COMMAND_LIST,
    },

//    VIEW {
//    },

  END_VIEW_LIST,
};


// Local static functions
static int create_listen_unix_sock(const char *path);


// Main loop events
static bool_t stop_loop_ev(faux_eloop_t *eloop, faux_eloop_type_e type,
	void *associated_data, void *user_data);
static bool_t refresh_config_ev(faux_eloop_t *eloop, faux_eloop_type_e type,
	void *associated_data, void *user_data);
static bool_t client_ev(faux_eloop_t *eloop, faux_eloop_type_e type,
	void *associated_data, void *user_data);
static bool_t listen_socket_ev(faux_eloop_t *eloop, faux_eloop_type_e type,
	void *associated_data, void *user_data);
static bool_t sched_once(faux_eloop_t *eloop, faux_eloop_type_e type,
	void *associated_data, void *user_data);
static bool_t sched_periodic(faux_eloop_t *eloop, faux_eloop_type_e type,
	void *associated_data, void *user_data);


/** @brief Main function
 */
int main(int argc, char **argv)
{
	int retval = -1;
	struct options *opts = NULL;
	int pidfd = -1;
	int logoptions = 0;
	faux_eloop_t *eloop = NULL;
	int listen_unix_sock = -1;
	ktpd_clients_t *clients = NULL;
	kscheme_t *scheme = NULL;

	struct timespec delayed = { .tv_sec = 10, .tv_nsec = 0 };
	struct timespec period = { .tv_sec = 3, .tv_nsec = 0 };

	// Parse command line options
	opts = opts_init();
	if (opts_parse(argc, argv, opts))
		goto err;

	// Initialize syslog
	logoptions = LOG_CONS;
	if (opts->foreground)
		logoptions |= LOG_PERROR;
	openlog(LOG_NAME, logoptions, opts->log_facility);
	if (!opts->verbose)
		setlogmask(LOG_UPTO(LOG_INFO));

	// Parse config file
	syslog(LOG_DEBUG, "Parse config file: %s\n", opts->cfgfile);
	if (!access(opts->cfgfile, R_OK)) {
		if (config_parse(opts->cfgfile, opts))
			goto err;
	} else if (opts->cfgfile_userdefined) {
		// User defined config must be found
		fprintf(stderr, "Error: Can't find config file %s\n",
			opts->cfgfile);
		goto err;
	}

	// DEBUG: Show options
	opts_show(opts);

	syslog(LOG_INFO, "Start daemon.\n");

	// Fork the daemon
	if (!opts->foreground) {
		// Daemonize
		syslog(LOG_DEBUG, "Daemonize\n");
		if (daemon(0, 0) < 0) {
			syslog(LOG_ERR, "Can't daemonize\n");
			goto err;
		}

		// Write pidfile
		syslog(LOG_DEBUG, "Write PID file: %s\n", opts->pidfile);
		if ((pidfd = open(opts->pidfile,
			O_WRONLY | O_CREAT | O_EXCL | O_TRUNC,
			S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0) {
			syslog(LOG_WARNING, "Can't open pidfile %s: %s\n",
				opts->pidfile, strerror(errno));
		} else {
			char str[20];
			snprintf(str, sizeof(str), "%u\n", getpid());
			str[sizeof(str) - 1] = '\0';
			if (write(pidfd, str, strlen(str)) < 0)
				syslog(LOG_WARNING, "Can't write to %s: %s\n",
					opts->pidfile, strerror(errno));
			close(pidfd);
		}
	}

	// Load scheme
	{
	char *txt = NULL;
	faux_error_t *error = faux_error_new();
	scheme = kscheme_from_ischeme(&sch, error);
	if (!scheme) {
		fprintf(stderr, "Scheme errors:\n");
		faux_error_print(error);
		goto err;
	}
	txt = ischeme_to_text(&sch, 0);
	printf("%s\n", txt);
	faux_str_free(txt);
	}

	// Listen socket
	syslog(LOG_DEBUG, "Create listen UNIX socket: %s\n", opts->unix_socket_path);
	listen_unix_sock = create_listen_unix_sock(opts->unix_socket_path);
	if (listen_unix_sock < 0)
		goto err;
	syslog(LOG_DEBUG, "Listen socket %d", listen_unix_sock);

	// Clients sessions DB
	clients = ktpd_clients_new();
	assert(clients);
	if (!clients)
		goto err;

	// Event loop
	eloop = faux_eloop_new(NULL);
	// Signals
	faux_eloop_add_signal(eloop, SIGINT, stop_loop_ev, NULL);
	faux_eloop_add_signal(eloop, SIGTERM, stop_loop_ev, NULL);
	faux_eloop_add_signal(eloop, SIGQUIT, stop_loop_ev, NULL);
	faux_eloop_add_signal(eloop, SIGHUP, refresh_config_ev, opts);
	// Listen socket. Waiting for new connections
	faux_eloop_add_fd(eloop, listen_unix_sock, POLLIN, listen_socket_ev, clients);
	// Scheduled events
	faux_eloop_add_sched_once_delayed(eloop, &delayed, 1, sched_once, NULL);
	faux_eloop_add_sched_periodic_delayed(eloop, 2, sched_periodic, NULL, &period, FAUX_SCHED_INFINITE);
	// Main loop
	faux_eloop_loop(eloop);
	faux_eloop_free(eloop);

/*
		// Non-blocking wait for all children
		while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
			syslog(LOG_DEBUG, "Exit child process %d\n", pid);
		}
*/

	retval = 0;

err:
	syslog(LOG_DEBUG, "Cleanup.\n");

	ktpd_clients_free(clients);

	// Close listen socket
	if (listen_unix_sock >= 0)
		close(listen_unix_sock);

	// Remove pidfile
	if (pidfd >= 0) {
		if (unlink(opts->pidfile) < 0) {
			syslog(LOG_ERR, "Can't remove pid-file %s: %s\n",
			opts->pidfile, strerror(errno));
		}
	}

	// Free scheme
	kscheme_free(scheme);

	// Free command line options
	opts_free(opts);
	syslog(LOG_INFO, "Stop daemon.\n");

	return retval;
}


/** @brief Create listen socket
 *
 * Previously removes old socket's file from filesystem. Note daemon must check
 * for already working daemon to don't duplicate.
 *
 * @param [in] path Socket path within filesystem.
 * @return Socket descriptor of < 0 on error.
 */
static int create_listen_unix_sock(const char *path)
{
	int sock = -1;
	int opt = 1;
	struct sockaddr_un laddr = {};

	assert(path);
	if (!path)
		return -1;

	if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		syslog(LOG_ERR, "Can't create socket: %s\n", strerror(errno));
		goto err;
	}
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
		syslog(LOG_ERR, "Can't set socket options: %s\n", strerror(errno));
		goto err;
	}

	// Remove old (lost) socket's file
	unlink(path);

	laddr.sun_family = AF_UNIX;
	strncpy(laddr.sun_path, path, USOCK_PATH_MAX);
	laddr.sun_path[USOCK_PATH_MAX - 1] = '\0';
	if (bind(sock, (struct sockaddr *)&laddr, sizeof(laddr))) {
		syslog(LOG_ERR, "Can't bind socket %s: %s\n", path, strerror(errno));
		goto err;
	}

	if (listen(sock, 128)) {
		unlink(path);
		syslog(LOG_ERR, "Can't listen on socket %s: %s\n", path, strerror(errno));
		goto err;
	}

	return sock;

err:
	if (sock >= 0)
		close(sock);

	return -1;
}


/** @brief Stop main event loop.
 */
static bool_t stop_loop_ev(faux_eloop_t *eloop, faux_eloop_type_e type,
	void *associated_data, void *user_data)
{
	// Happy compiler
	eloop = eloop;
	type = type;
	associated_data = associated_data;
	user_data = user_data;

	return BOOL_FALSE; // Stop Event Loop
}


/** @brief Re-read config file.
 */
static bool_t refresh_config_ev(faux_eloop_t *eloop, faux_eloop_type_e type,
	void *associated_data, void *user_data)
{
	struct options *opts = (struct options *)user_data;

	if (access(opts->cfgfile, R_OK) == 0) {
		syslog(LOG_DEBUG, "Re-reading config file \"%s\"\n", opts->cfgfile);
		if (config_parse(opts->cfgfile, opts) < 0)
			syslog(LOG_ERR, "Error while config file parsing.\n");
	} else if (opts->cfgfile_userdefined) {
		syslog(LOG_ERR, "Can't find config file \"%s\"\n", opts->cfgfile);
	}

	// Happy compiler
	eloop = eloop;
	type = type;
	associated_data = associated_data;

	return BOOL_TRUE;
}


bool_t fd_stall_cb(ktpd_session_t *session, void *user_data)
{
	faux_eloop_t *eloop = (faux_eloop_t *)user_data;

	assert(session);
	assert(eloop);

	faux_eloop_include_fd_event(eloop, ktpd_session_fd(session), POLLOUT);

	return BOOL_TRUE;
}


/** @brief Event on listen socket. New remote client.
 */
static bool_t listen_socket_ev(faux_eloop_t *eloop, faux_eloop_type_e type,
	void *associated_data, void *user_data)
{
	int new_conn = -1;
	faux_eloop_info_fd_t *info = (faux_eloop_info_fd_t *)associated_data;
	ktpd_clients_t *clients = (ktpd_clients_t *)user_data;
	ktpd_session_t *session = NULL;

	assert(clients);

	new_conn = accept(info->fd, NULL, NULL);
	if (new_conn < 0) {
		syslog(LOG_ERR, "Can't accept() new connection");
		return BOOL_TRUE;
	}
	session = ktpd_clients_add(clients, new_conn);
	if (!session) {
		syslog(LOG_ERR, "Duplicated client fd");
		close(new_conn);
		return BOOL_TRUE;
	}
	ktpd_session_set_stall_cb(session, fd_stall_cb, eloop);
	faux_eloop_add_fd(eloop, new_conn, POLLIN, client_ev, clients);
	syslog(LOG_DEBUG, "New connection %d", new_conn);

	type = type; // Happy compiler
	user_data = user_data; // Happy compiler

	return BOOL_TRUE;
}


static bool_t client_ev(faux_eloop_t *eloop, faux_eloop_type_e type,
	void *associated_data, void *user_data)
{
	faux_eloop_info_fd_t *info = (faux_eloop_info_fd_t *)associated_data;
	ktpd_clients_t *clients = (ktpd_clients_t *)user_data;
	ktpd_session_t *session = NULL;

	assert(clients);

	// Find out session
	session = ktpd_clients_find(clients, info->fd);
	if (!session) { // Some strange case
		syslog(LOG_ERR, "Can't find client session for fd %d", info->fd);
		faux_eloop_del_fd(eloop, info->fd);
		close(info->fd);
		return BOOL_TRUE;
	}

	// Write data
	if (info->revents & POLLOUT) {
		faux_eloop_exclude_fd_event(eloop, info->fd, POLLOUT);
		if (!ktpd_session_async_out(session)) {
			// Someting went wrong
			faux_eloop_del_fd(eloop, info->fd);
			ktpd_clients_del(clients, info->fd);
			syslog(LOG_ERR, "Problem with async input");
		}
	}

	// Read data
	if (info->revents & POLLIN) {
		if (!ktpd_session_async_in(session)) {
			// Someting went wrong
			faux_eloop_del_fd(eloop, info->fd);
			ktpd_clients_del(clients, info->fd);
			syslog(LOG_ERR, "Problem with async input");
		}
	}

	// EOF
	if (info->revents & POLLHUP) {
		faux_eloop_del_fd(eloop, info->fd);
		ktpd_clients_del(clients, info->fd);
		syslog(LOG_DEBUG, "Close connection %d", info->fd);
	}

	type = type; // Happy compiler
	user_data = user_data; // Happy compiler

	return BOOL_TRUE;
}


static bool_t sched_once(faux_eloop_t *eloop, faux_eloop_type_e type,
	void *associated_data, void *user_data)
{
	faux_eloop_info_sched_t *info = (faux_eloop_info_sched_t *)associated_data;
printf("Once %d\n", info->ev_id);

	// Happy compiler
	eloop = eloop;
	type = type;
	associated_data = associated_data;
	user_data = user_data;

	return BOOL_TRUE;
}

static bool_t sched_periodic(faux_eloop_t *eloop, faux_eloop_type_e type,
	void *associated_data, void *user_data)
{
	faux_eloop_info_sched_t *info = (faux_eloop_info_sched_t *)associated_data;
printf("Periodic %d\n", info->ev_id);

	// Happy compiler
	eloop = eloop;
	type = type;
	associated_data = associated_data;
	user_data = user_data;

	return BOOL_TRUE;
}
