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

#include <klish/ktp.h>
#include <klish/ktp_session.h>

#include "private.h"

// Signal handlers
static volatile int sigterm = 0; // Exit if 1
static void sighandler(int signo);
static volatile int sighup = 0; // Re-read config file
static void sighup_handler(int signo);
static volatile int sigchld = 0; // Child execution is finished
static void sigchld_handler(int signo);

// Network
static int create_listen_unix_sock(const char *path);


/** @brief Main function
 */
int main(int argc, char **argv)
{
	int retval = -1;
	struct options *opts = NULL;
	int pidfd = -1;
	int logoptions = 0;

	// Network
	int listen_unix_sock = -1;
	faux_pollfd_t *fds = NULL;

	// Event scheduler
	faux_sched_t *sched = NULL;

	// Signal vars
	struct sigaction sig_act = {};
	sigset_t sig_set = {};
	sigset_t orig_sig_set = {}; // Saved signal mask

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

	// Set signal handler
	syslog(LOG_DEBUG, "Set signal handlers\n");
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

	// SIGHUP handler
	sigemptyset(&sig_set);
	sigaddset(&sig_set, SIGHUP);

	sig_act.sa_flags = 0;
	sig_act.sa_mask = sig_set;
	sig_act.sa_handler = &sighup_handler;
	sigaction(SIGHUP, &sig_act, NULL);

	// SIGCHLD handler
	sigemptyset(&sig_set);
	sigaddset(&sig_set, SIGCHLD);

	sig_act.sa_flags = 0;
	sig_act.sa_mask = sig_set;
	sig_act.sa_handler = &sigchld_handler;
	sigaction(SIGCHLD, &sig_act, NULL);

	// Initialize event scheduler
	sched = faux_sched_new();
	if (!sched) {
		syslog(LOG_ERR, "Can't init event scheduler");
		goto err;
	}

	// The struct pollfd vector for ppoll()
	fds = faux_pollfd_new();
	if (!fds) {
		syslog(LOG_ERR, "Can't init pollfd vector");
		goto err;
	}
	// Add listen UNIX socket to pollfds vector
	faux_pollfd_add(fds, listen_unix_sock, POLLIN);

	// Block signals to prevent race conditions while loop and ppoll()
	// Catch signals while ppoll() only
	sigemptyset(&sig_set);
	sigaddset(&sig_set, SIGTERM);
	sigaddset(&sig_set, SIGINT);
	sigaddset(&sig_set, SIGQUIT);
	sigaddset(&sig_set, SIGHUP);
	sigaddset(&sig_set, SIGCHLD);
	sigprocmask(SIG_BLOCK, &sig_set, &orig_sig_set);

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
				syslog(LOG_DEBUG, "New connection %d\n", new_conn);
				continue;
			}

			// If it's not a listen socket then we have received
			// a message from client.
		}

	} // Main loop end


	retval = 0;

err:
	syslog(LOG_DEBUG, "Cleanup.\n");

	sigprocmask(SIG_BLOCK, &orig_sig_set, NULL);
	faux_pollfd_free(fds);
	faux_sched_free(sched);

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


/** @brief Signal handler for temination signals (like SIGTERM, SIGINT, ...)
 */
static void sighandler(int signo)
{
	sigterm = 1;
	signo = signo; // Happy compiler
}


/** @brief Re-read config file on SIGHUP
 */
static void sighup_handler(int signo)
{
	sighup = 1;
	signo = signo; // Happy compiler
}


/** @brief Child was finished
 */
static void sigchld_handler(int signo)
{
	sigchld = 1;
	signo = signo; // Happy compiler
}
