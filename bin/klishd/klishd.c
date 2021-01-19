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

#include <klish/ktp.h>
#include <klish/ktp_session.h>

#include "private.h"

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

	// Network
	int listen_unix_sock = -1;

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

	// Network initialization
	syslog(LOG_DEBUG, "Create listen UNIX socket: %s\n", opts->unix_socket_path);
	listen_unix_sock = create_listen_unix_sock(opts->unix_socket_path);
	if (listen_unix_sock < 0)
		goto err;
	syslog(LOG_DEBUG, "Listen socket %d", listen_unix_sock);

	eloop = faux_eloop_new(NULL);
	// Signals
	faux_eloop_add_signal(eloop, SIGINT, stop_loop_ev, NULL);
	faux_eloop_add_signal(eloop, SIGTERM, stop_loop_ev, NULL);
	faux_eloop_add_signal(eloop, SIGQUIT, stop_loop_ev, NULL);
	faux_eloop_add_signal(eloop, SIGHUP, refresh_config_ev, opts);
	// Listen socket. Waiting for new connections
	faux_eloop_add_fd(eloop, listen_unix_sock, POLLIN, listen_socket_ev, NULL);
	// Scheduled events
	faux_eloop_add_sched_once_delayed(eloop, &delayed, 1, sched_once, NULL);
	faux_eloop_add_sched_periodic_delayed(eloop, 2, sched_periodic, NULL, &period, FAUX_SCHED_INFINITE);
	// Main loop
	faux_eloop_loop(eloop);
	faux_eloop_free(eloop);

/*
	// Main loop
	while (!sigterm) {
		int sn = 0;
		struct timespec *timeout = NULL;
		struct timespec next_interval = {};
		faux_pollfd_iterator_t pollfd_iter;
		struct pollfd *pollfd = NULL;
		pid_t pid = -1;

		// Re-read config file on SIGHUP
		if (sighup) {
			if (access(opts->cfgfile, R_OK) == 0) {
				syslog(LOG_INFO, "Re-reading config file \"%s\"\n", opts->cfgfile);
				if (config_parse(opts->cfgfile, opts) < 0)
					syslog(LOG_ERR, "Error while config file parsing.\n");
			} else if (opts->cfgfile_userdefined) {
				syslog(LOG_ERR, "Can't find config file \"%s\"\n", opts->cfgfile);
			}
			sighup = 0;
		}

		// Non-blocking wait for all children
		while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
			syslog(LOG_DEBUG, "Exit child process %d\n", pid);
		}
		sigchld = 0;

		// Find out timeout interval
		if (faux_sched_next_interval(sched, &next_interval) < 0) {
			timeout = NULL;
		} else {
			timeout = &next_interval;
			syslog(LOG_DEBUG, "Next interval: %ld\n", timeout->tv_sec);
		}

		// Wait for events
		sn = ppoll(faux_pollfd_vector(fds), faux_pollfd_len(fds), timeout, &orig_sig_set);
		if (sn < 0) {
			if ((EAGAIN == errno) || (EINTR == errno))
				continue;
			syslog(LOG_ERR, "Error while select(): %s\n", strerror(errno));
			break;
		}

		// Timeout (Scheduled event)
		if (0 == sn) {
			int id = 0; // Event idenftifier
			void *data = NULL; // Event data

syslog(LOG_DEBUG, "Timeout\n");
			// Some scheduled events
			while(faux_sched_pop(sched, &id, &data) == 0) {
				syslog(LOG_DEBUG, "sched: Update event\n");
			}
			continue;
		}

		// Get data via socket
		faux_pollfd_init_iterator(fds, &pollfd_iter);
		while ((pollfd = faux_pollfd_each_active(fds, &pollfd_iter))) {
			int fd = pollfd->fd;

			// Listen socket
			if (fd == listen_unix_sock) {
				int new_conn = -1;
				new_conn = accept(listen_unix_sock, NULL, NULL);
				if (new_conn < 0)
					continue;
				faux_pollfd_add(fds, new_conn, POLLIN);
				syslog(LOG_DEBUG, "New connection %d", new_conn);
				continue;
			}

			// If it's not a listen socket then we have received
			// a message from client.
syslog(LOG_DEBUG, "Client %d\n", fd);

			// Receive message
			
			

			// Check for closed connection. Do it after reading from
			// socket because buffer of disconnected socket can
			// still contain data.
			if (pollfd->revents & POLLHUP) {
				ktp_disconnect(fd);
				faux_pollfd_del_by_fd(fds, fd);
				syslog(LOG_DEBUG, "Close connection %d", fd);
			}

		}

	} // Main loop end
*/

	retval = 0;

err:
	syslog(LOG_DEBUG, "Cleanup.\n");

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


/** @brief Event on listen socket. New remote client.
 */
static bool_t listen_socket_ev(faux_eloop_t *eloop, faux_eloop_type_e type,
	void *associated_data, void *user_data)
{
	int new_conn = -1;
	faux_eloop_info_fd_t *info = (faux_eloop_info_fd_t *)associated_data;

	new_conn = accept(info->fd, NULL, NULL);
	if (new_conn < 0) {
		syslog(LOG_ERR, "Can't accept() new connection");
		return BOOL_TRUE;
	}
	faux_eloop_add_fd(eloop, new_conn, POLLIN, client_ev, NULL);
	syslog(LOG_DEBUG, "New connection %d", new_conn);

	type = type; // Happy compiler
	user_data = user_data; // Happy compiler

	return BOOL_TRUE;
}


static bool_t client_ev(faux_eloop_t *eloop, faux_eloop_type_e type,
	void *associated_data, void *user_data)
{
	faux_eloop_info_fd_t *info = (faux_eloop_info_fd_t *)associated_data;

	if (info->revents & POLLIN) {
		char buf[1000];
		ssize_t s = 0;

		s = read(info->fd, buf, 1000);
printf("Received %ld bytes on fd %d\n", s, info->fd);
//faux_eloop_add_signal(eloop, SIGINT, stop_loop_ev, NULL);
	}

	if (info->revents & POLLHUP) {
		close(info->fd);
		faux_eloop_del_fd(eloop, info->fd);
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
