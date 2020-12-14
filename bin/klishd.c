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
#include <getopt.h>
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
#include <crsp.h>
#include <crsp_cdp.h>
#include <crsp_hl.h>

#ifndef VERSION
#define VERSION "1.0.0"
#endif

#define CRSD_LOG_NAME "crsd"
#define CRSD_DEFAULT_PIDFILE "/var/run/crsd.pid"
#define CRSD_DEFAULT_CFGFILE "/etc/crs/crsd.conf"
#define CRSD_DEFAULT_FETCH_INTERVAL 120
#define CRSD_DEFAULT_FETCH_MARGIN 50
#define CRSD_DEFAULT_FETCH_TIMEOUT 60
#define CRSD_DEFAULT_CRL_MAX_SIZE 50000000ul
#define CRSD_DEFAULT_CRL_STORAGE_MAX_SIZE 5000000000ul
#define CRSD_DEFAULT_ROOTCERT_DIR "/etc/config/ike/ipsec.d/rootcerts"
#define CRSD_DEFAULT_CACERT_DIR "/etc/config/ike/ipsec.d/cacerts"
#define CRSD_DEFAULT_CERT_DIR "/etc/config/ike/ipsec.d/certs"
#define CRSD_DEFAULT_OCSPCERT_DIR  "/etc/config/ike/ipsec.d/ocspcerts"
#define CRSD_DEFAULT_CRL_DIR "/etc/config/ike/ipsec.d/crls"
#define CRSD_DEFAULT_KEY_DIR "/etc/config/ike/ipsec.d/private"
#define CRSD_DEFAULT_P10_DIR "/etc/config/ike/ipsec.d/reqs"
#define CRSD_DEFAULT_SECRETS_FILE "/etc/config/ike/ipsec.secrets"

// Signal handlers
static volatile int sigterm = 0; // Exit if 1
static void sighandler(int signo);
static volatile int sighup = 0; // Re-read config file
static void sighup_handler(int signo);
static volatile int sigchld = 0; // Child execution is finished
static void sigchld_handler(int signo);

// Options and config file
static void help(int status, const char *argv0);
static struct options *opts_init(void);
static void opts_free(struct options *opts);
static int opts_parse(int argc, char *argv[], struct options *opts);
static int opts_show(struct options *opts);
static int config_parse(const char *cfgfile, struct options *opts);


// Local syslog function
bool_t verbose = BOOL_FALSE;
bool_t silent = BOOL_FALSE;
static void lsyslog(int priority, const char *format, ...)
{
	va_list ap;

	if (silent)
		return;

	if (!verbose && (LOG_DEBUG == priority))
		return;

	// Calculate buffer size
	va_start(ap, format);
	vsyslog(priority, format, ap);
	va_end(ap);
}


/** @brief Command line and config file options
 */
struct options {
	char *pidfile;
	char *cfgfile;
	char *unix_socket_path;
	bool_t cfgfile_userdefined;
	bool_t foreground; // Don't daemonize
	bool_t verbose;
	bool_t silent;
	bool_t check;
	int log_facility;
	char *uid_str;
	char *gid_str;
	uid_t uid;
	gid_t gid;
	long int fetch_interval;
	unsigned char fetch_margin;
	long int fetch_timeout;
	unsigned long int crl_max_size;
	unsigned long int crl_storage_max_size;
	char *rootcert_dir;
	char *cacert_dir;
	char *cert_dir;
	char *ocspcert_dir;
	char *crl_dir;
	char *key_dir;
	char *p10_dir;
	char *secrets_file;
};

// Network
static int create_listen_sock(const char *path, uid_t uid, gid_t gid);


/** @brief Main function
 */
int main(int argc, char **argv)
{
	int retval = -1;
	struct options *opts = NULL;
	int pidfd = -1;
	int logmask = 0;
	int logoptions = 0;

	// Event scheduler
	faux_sched_t *sched = NULL;

	// Network
	int listen_sock = -1;
	faux_pollfd_t *fds = NULL;

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
	openlog(CRSD_LOG_NAME, logoptions, opts->log_facility);
	if (!opts->verbose) {
		logmask = setlogmask(0);
		logmask = logmask & ~LOG_MASK(LOG_DEBUG);
		setlogmask(logmask);
	}

	// Parse config file
	lsyslog(LOG_DEBUG, "Parse config file: %s\n", opts->cfgfile);
	if (!access(opts->cfgfile, R_OK)) {
		if (config_parse(opts->cfgfile, opts))
			goto err;
	} else if (opts->cfgfile_userdefined) {
		// User defined config must be found
		fprintf(stderr, "Error: Can't find config file %s\n",
			opts->cfgfile);
		goto err;
	}
	if (opts->check) { // Check only. Return 0 if config file is ok.
		retval = 0;
		goto err;
	}

	// DEBUG: Show options
	opts_show(opts);

	// Set crsp lib global var for debug
	if (opts->verbose)
		crsp_debug = 1;

	lsyslog(LOG_INFO, "Start daemon.\n");

	// Fork the daemon
	if (!opts->foreground) {
		// Daemonize
		lsyslog(LOG_DEBUG, "Daemonize\n");
		if (daemon(0, 0) < 0) {
			lsyslog(LOG_ERR, "Can't daemonize\n");
			goto err;
		}

		// Write pidfile
		lsyslog(LOG_DEBUG, "Write PID file: %s\n", opts->pidfile);
		if ((pidfd = open(opts->pidfile,
			O_WRONLY | O_CREAT | O_EXCL | O_TRUNC,
			S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0) {
			lsyslog(LOG_WARNING, "Can't open pidfile %s: %s\n",
				opts->pidfile, strerror(errno));
		} else {
			char str[20];
			snprintf(str, sizeof(str), "%u\n", getpid());
			str[sizeof(str) - 1] = '\0';
			if (write(pidfd, str, strlen(str)) < 0)
				lsyslog(LOG_WARNING, "Can't write to %s: %s\n",
					opts->pidfile, strerror(errno));
			close(pidfd);
		}
	}

	// Network initialization
	lsyslog(LOG_DEBUG, "Create listen socket: %s\n", opts->unix_socket_path);
	listen_sock = create_listen_sock(opts->unix_socket_path,
		opts->uid, opts->gid);
	if (listen_sock < 0)
		goto err;

	// Change UID/GID
	if (opts->uid != getuid()) {
		lsyslog(LOG_DEBUG, "Change UID: %s\n", opts->uid_str ? opts->uid_str : "");
		setfsuid(opts->uid);
		if (setresuid(opts->uid, opts->uid, opts->uid) < 0) {
			lsyslog(LOG_ERR, "Can't change user: %s", strerror(errno));
			goto err;
		}
	}
	if (opts->gid != getgid()) {
		lsyslog(LOG_DEBUG, "Change GID: %s\n", opts->gid_str ? opts->gid_str : "");
		if (setresgid(opts->gid, opts->gid, opts->gid) < 0 ) {
			lsyslog(LOG_ERR, "Can't change group: %s", strerror(errno));
			goto err;
		}
	}

	// Set signal handler
	lsyslog(LOG_DEBUG, "Set signal handlers\n");
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

	// The struct pollfd vector for ppoll()
	fds = faux_pollfd_new();
	if (!fds) {
		lsyslog(LOG_ERR, "Can't init pollfd vector");
		goto err;
	}
	// Add listen socket to pollfds vector
	faux_pollfd_add(fds, listen_sock, POLLIN);

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
				lsyslog(LOG_INFO, "Re-reading config file\n");
				if (config_parse(opts->cfgfile, opts) < 0)
					lsyslog(LOG_ERR, "Error while config file parsing.\n");
				reschedule_all(sched, cdp_db,
					opts->fetch_interval,
					opts->fetch_margin);
			} else if (opts->cfgfile_userdefined) {
				lsyslog(LOG_ERR, "Can't find config file.\n");
			}
			sighup = 0;
		}

		// Non-blocking wait for all children
		while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
			lsyslog(LOG_DEBUG, "Exit child process %d\n", pid);
		}
		sigchld = 0;

		// Find out timeout interval
		if (faux_sched_next_interval(sched, &next_interval) < 0) {
			timeout = NULL;
		} else {
			timeout = &next_interval;
			lsyslog(LOG_DEBUG, "Next interval: %ld\n", timeout->tv_sec);
		}

		// Wait for events
		sn = ppoll(faux_pollfd_vector(fds), faux_pollfd_len(fds), timeout, &orig_sig_set);
		if (sn < 0) {
			if ((EAGAIN == errno) || (EINTR == errno))
				continue;
			lsyslog(LOG_ERR, "Error while select(): %s\n", strerror(errno));
			break;
		}

		// Timeout (Scheduled event)
		if (0 == sn) {
			int id = 0; // Event idenftifier
			void *data = NULL; // Event data

			// Some scheduled events
			while(faux_sched_pop(sched, &id, &data) == 0) {
				cdp_t *cdp = (cdp_t *)data;
				download_args_t download_args = {};
				// Now we have only one type of event - it's
				// a scheduled CRL downloading
				lsyslog(LOG_DEBUG, "sched: Update event for %s\n", cdp_uri(cdp));
				download_args.cdp = cdp;
				download_args.opts = opts;
				fork_action(download_crl, (void *)&download_args);
			}
			continue;
		}

		// Get data via socket
		faux_pollfd_init_iterator(fds, &pollfd_iter);
		while ((pollfd = faux_pollfd_each_active(fds, &pollfd_iter))) {
			crsp_msg_t *req = NULL;
			crsp_msg_t *ack = NULL;
			faux_net_t *net = NULL;
			int fd = pollfd->fd;
			crsp_recv_e recv_status = CRSP_RECV_OK;

			// Listen socket
			if (fd == listen_sock) {
				int new_conn = -1;
				new_conn = accept(listen_sock, NULL, NULL);
				if (new_conn < 0)
					continue;
				faux_pollfd_add(fds, new_conn, POLLIN);
				lsyslog(LOG_DEBUG, "New connection %d\n", new_conn);
				continue;
			}

			// If it's not a listen socket then we have received
			// a message from client.
			net = faux_net_new();
			faux_net_set_fd(net, fd);
			// Get message
			req = crsp_msg_recv(net, &recv_status);

			// Prepare empty answer structure
			ack = crsp_msg_new();

			// Compose answer
			if (req) {
				msg_cb_f *handler =
					msg_handler[crsp_msg_get_cmd(req)];
				crsp_msg_t *answer = NULL;

				// Callback function fills the answer by
				// real data
				if (handler) {
					msg_cb_args_t args = {};

					args.cdp_db = cdp_db;
					args.sched = sched;
					args.opts = opts;
					args.credio = credio;
					args.net = net;
					answer = handler(req, ack, &args);
					if (!answer) { // Some local problem
						crsp_msg_set_cmd(ack, CRSP_CMD_ERROR);
						crsp_msg_set_status(ack, CRSP_STATUS_ERROR);
					}
					cdp_db_debug(cdp_db);
				// If there is no handler for this commnand just
				// return "unknown/unsupported command" message.
				} else {
					crsp_msg_set_cmd(ack, CRSP_CMD_ERROR);
					crsp_msg_set_status(ack, CRSP_STATUS_UNSUPPORTED);
				}

			// Some error while receiving
			} else {
				crsp_msg_set_cmd(ack, CRSP_CMD_ERROR);
				switch(recv_status) {
				case CRSP_RECV_OK: // Some system problem
					crsp_msg_set_status(ack, CRSP_STATUS_ERROR);
					break;
				default: // Classified problem
					// Now error codes of status is
					// equal to receive statuses. It can be
					// changed later.
					crsp_msg_set_status(ack, recv_status);
					break;
				}
			}

			// Send ACK
			crsp_msg_send(ack, net);
			crsp_msg_free(ack);

			crsp_msg_free(req);
			close(fd);
			faux_net_free(net);
			faux_pollfd_del_by_fd(fds, fd);
			lsyslog(LOG_DEBUG, "Close connection %d\n", fd);
		}

	} // Main loop end


	retval = 0;

err:
	lsyslog(LOG_DEBUG, "Cleanup.\n");

	sigprocmask(SIG_BLOCK, &orig_sig_set, NULL);
	faux_pollfd_free(fds);
	faux_sched_free(sched);

	// Close listen socket
	if (listen_sock >= 0)
		close(listen_sock);

	// Clean CDP database
	cdp_db_free(cdp_db);

	// Remove pidfile
	if (pidfd >= 0) {
		if (unlink(opts->pidfile) < 0) {
			lsyslog(LOG_ERR, "Can't remove pid-file %s: %s\n",
			opts->pidfile, strerror(errno));
		}
	}

	// Crypto cleanup
	pgst_credio_free(&credio);
	pgst_cossl_free(&ossl);

	// Free command line options
	opts_free(opts);
	lsyslog(LOG_INFO, "Stop daemon.\n");

	return retval;
}


/** @brief Create listen socket
 */
static int create_listen_sock(const char *path, uid_t uid, gid_t gid)
{
	int sock = -1;
	int opt = 1;
	struct sockaddr_un laddr = {};
//	mode_t mode = 0660;

	if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		lsyslog(LOG_ERR, "Cannot create socket: %s\n", strerror(errno));
		goto error;
	}
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
		lsyslog(LOG_ERR, "Can't set socket options: %s\n", strerror(errno));
		goto error;
	}
	laddr.sun_family = AF_UNIX;
	strncpy(laddr.sun_path, path, USOCK_PATH_MAX);
	laddr.sun_path[USOCK_PATH_MAX - 1] = '\0';

	unlink(path);

	if (bind(sock, (struct sockaddr *)&laddr, sizeof(laddr))) {
		lsyslog(LOG_ERR, "Can't bind socket: %s\n", strerror(errno));
		goto err2;
	}

	if (uid != getuid() || gid != getgid()) {
		if (chown(path, uid, gid)) {
			lsyslog(LOG_ERR, "Can't chown socket: %s\n", strerror(errno));
			goto err2;
		}
	}

//	if (chmod(path, mode)) {
//		lsyslog(LOG_ERR, "Can't chmod socket: %s\n", strerror(errno));
//		goto err2;
//	}

	if (listen(sock, 64)) {
		lsyslog(LOG_ERR, "Can't listen socket: %s\n", strerror(errno));
		goto err2;
	}

	return sock;

err2:
	unlink(path);
error:
	if (sock != -1)
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


/** @brief Initialize option structure by defaults
 */
static struct options *opts_init(void)
{
	struct options *opts = NULL;

	opts = faux_zmalloc(sizeof(*opts));
	assert(opts);

	// Initialize
	opts->pidfile = faux_str_dup(CRSD_DEFAULT_PIDFILE);
	opts->cfgfile = faux_str_dup(CRSD_DEFAULT_CFGFILE);
	opts->unix_socket_path = faux_str_dup(CRSD_DEFAULT_UNIX_SOCKET_PATH);
	opts->cfgfile_userdefined = BOOL_FALSE;
	opts->foreground = BOOL_FALSE; // Daemonize by default
	opts->verbose = BOOL_FALSE;
	opts->silent = BOOL_FALSE;
	opts->check = BOOL_FALSE;
	opts->log_facility = LOG_DAEMON;
	opts->uid = getuid();
	opts->gid = getgid();
	opts->uid_str = faux_sysdb_name_by_uid(opts->uid);
	opts->gid_str = faux_sysdb_name_by_gid(opts->gid);
	opts->fetch_interval = CRSD_DEFAULT_FETCH_INTERVAL;
	opts->fetch_margin = CRSD_DEFAULT_FETCH_MARGIN;
	opts->fetch_timeout = CRSD_DEFAULT_FETCH_TIMEOUT;
	opts->crl_max_size = CRSD_DEFAULT_CRL_MAX_SIZE;
	opts->crl_storage_max_size = CRSD_DEFAULT_CRL_STORAGE_MAX_SIZE;
	opts->rootcert_dir = faux_str_dup(CRSD_DEFAULT_ROOTCERT_DIR);
	opts->cacert_dir = faux_str_dup(CRSD_DEFAULT_CACERT_DIR);
	opts->cert_dir = faux_str_dup(CRSD_DEFAULT_CERT_DIR);
	opts->ocspcert_dir = faux_str_dup(CRSD_DEFAULT_OCSPCERT_DIR);
	opts->crl_dir = faux_str_dup(CRSD_DEFAULT_CRL_DIR);
	opts->key_dir = faux_str_dup(CRSD_DEFAULT_KEY_DIR);
	opts->p10_dir = faux_str_dup(CRSD_DEFAULT_P10_DIR);
	opts->secrets_file = faux_str_dup(CRSD_DEFAULT_SECRETS_FILE);

	return opts;
}


/** @brief Free options structure
 */
static void opts_free(struct options *opts)
{
	faux_str_free(opts->pidfile);
	faux_str_free(opts->cfgfile);
	faux_str_free(opts->unix_socket_path);
	faux_str_free(opts->uid_str);
	faux_str_free(opts->gid_str);
	faux_str_free(opts->rootcert_dir);
	faux_str_free(opts->cacert_dir);
	faux_str_free(opts->cert_dir);
	faux_str_free(opts->ocspcert_dir);
	faux_str_free(opts->crl_dir);
	faux_str_free(opts->key_dir);
	faux_str_free(opts->p10_dir);
	faux_str_free(opts->secrets_file);
	faux_free(opts);
}


/** @brief Parse command line options
 */
static int opts_parse(int argc, char *argv[], struct options *opts)
{
	static const char *shortopts = "hp:c:fl:vsk";
	static const struct option longopts[] = {
		{"help",		0, NULL, 'h'},
		{"pid",			1, NULL, 'p'},
		{"conf",		1, NULL, 'c'},
		{"foreground",		0, NULL, 'f'},
		{"verbose",		0, NULL, 'v'},
		{"silent",		0, NULL, 's'},
		{"check",		0, NULL, 'k'},
		{"facility",		1, NULL, 'l'},
		{NULL,			0, NULL, 0}
	};

	optind = 1;
	while(1) {
		int opt = 0;

		opt = getopt_long(argc, argv, shortopts, longopts, NULL);
		if (-1 == opt)
			break;
		switch (opt) {
		case 'p':
			faux_str_free(opts->pidfile);
			opts->pidfile = faux_str_dup(optarg);
			break;
		case 'c':
			faux_str_free(opts->cfgfile);
			opts->cfgfile = faux_str_dup(optarg);
			opts->cfgfile_userdefined = BOOL_TRUE;
			break;
		case 'f':
			opts->foreground = BOOL_TRUE;
			break;
		case 'v':
			opts->verbose = BOOL_TRUE;
			break;
		case 's':
			opts->silent = BOOL_TRUE;
			break;
		case 'k':
			opts->check = BOOL_TRUE;
			break;
		case 'l':
			if (faux_log_facility_id(optarg, &(opts->log_facility))) {
				fprintf(stderr, "Error: Illegal syslog facility %s.\n", optarg);
				_exit(-1);
			}
			break;
		case 'h':
			help(0, argv[0]);
			_exit(0);
			break;
		default:
			help(-1, argv[0]);
			_exit(-1);
			break;
		}
	}

	return 0;
}

/** @brief Print help message
 */
static void help(int status, const char *argv0)
{
	const char *name = NULL;

	if (!argv0)
		return;

	// Find the basename
	name = strrchr(argv0, '/');
	if (name)
		name++;
	else
		name = argv0;

	if (status != 0) {
		fprintf(stderr, "Try `%s -h' for more information.\n",
			name);
	} else {
		printf("Version : %s\n", VERSION);
		printf("Usage   : %s [options]\n", name);
		printf("Certificate Revocation Service Daemon\n");
		printf("Options :\n");
		printf("\t-h, --help Print this help.\n");
		printf("\t-f, --foreground Don't daemonize.\n");
		printf("\t-v, --verbose Be verbose.\n");
		printf("\t-s, --silent Be silent.\n");
		printf("\t-k, --check Check config only and exit.\n");
		printf("\t-p <path>, --pid=<path> File to save daemon's PID to ("
			CRSD_DEFAULT_PIDFILE ").\n");
		printf("\t-c <path>, --conf=<path> Config file ("
			CRSD_DEFAULT_CFGFILE ").\n");
		printf("\t-l, --facility Syslog facility (DAEMON).\n");
	}
}


/** @brief Parse config file
 */
static int config_parse(const char *cfgfile, struct options *opts)
{
	faux_ini_t *ini = NULL;
	const char *tmp = NULL;

	ini = faux_ini_new();
	assert(ini);
	if (!ini)
		return -1;
	if (faux_ini_parse_file(ini, cfgfile)) {
		lsyslog(LOG_ERR, "Can't parse config file: %s\n", cfgfile);
		faux_ini_free(ini);
		return -1;
	}

	if ((tmp = faux_ini_find(ini, "UnixSocketPath"))) {
		faux_str_free(opts->unix_socket_path);
		opts->unix_socket_path = faux_str_dup(tmp);
	}

	if ((tmp = faux_ini_find(ini, "UID"))) {
		faux_str_free(opts->uid_str);
		opts->uid_str = faux_str_dup(tmp);
		if (faux_sysdb_uid_by_name(opts->uid_str, &opts->uid) < 0) {
			lsyslog(LOG_ERR, "Unknown user: %s\n", opts->uid_str);
			faux_ini_free(ini);
			return -1; // Unknown user
		}
	}

	if ((tmp = faux_ini_find(ini, "GID"))) {
		faux_str_free(opts->gid_str);
		opts->gid_str = faux_str_dup(tmp);
		if (faux_sysdb_gid_by_name(opts->gid_str, &opts->gid)) {
			lsyslog(LOG_ERR, "Unknown group: %s\n", opts->gid_str);
			faux_ini_free(ini);
			return -1; // Unknown group
		}
	}

	if ((tmp = faux_ini_find(ini, "FetchInterval"))) {
		if (faux_conv_atol(tmp, &opts->fetch_interval, 10) < 0) {
			lsyslog(LOG_ERR, "Illegal FetchInterval: %s\n", tmp);
			faux_ini_free(ini);
			return -1;
		}
		if (opts->fetch_interval < 0) {
			lsyslog(LOG_ERR, "FetchInterval can't be less than zero: %s\n", tmp);
			faux_ini_free(ini);
			return -1;
		}
	}

	if ((tmp = faux_ini_find(ini, "FetchMargin"))) {
		if (faux_conv_atouc(tmp, &opts->fetch_margin, 10) < 0) {
			lsyslog(LOG_ERR, "Illegal FetchMargin: %s\n", tmp);
			faux_ini_free(ini);
			return -1;
		}
		if (opts->fetch_margin > 100) { // Value in percents
			lsyslog(LOG_ERR, "FetchMargin (in percents) can't be greater than 100: %s\n", tmp);
			faux_ini_free(ini);
			return -1;
		}
	}

	if ((tmp = faux_ini_find(ini, "FetchTimeout"))) {
		if (faux_conv_atol(tmp, &opts->fetch_timeout, 10) < 0) {
			lsyslog(LOG_ERR, "Illegal FetchTimeout: %s\n", tmp);
			faux_ini_free(ini);
			return -1;
		}
		if (opts->fetch_timeout < 0) {
			lsyslog(LOG_ERR, "FetchTimeout can't be less than zero: %s\n", tmp);
			faux_ini_free(ini);
			return -1;
		}
		if (opts->fetch_timeout > opts->fetch_interval) {
			lsyslog(LOG_ERR, "FetchTimeout can't be greater than FetchInterval: %s\n", tmp);
			faux_ini_free(ini);
			return -1;
		}
	}

	if ((tmp = faux_ini_find(ini, "CRLMaxSize"))) {
		if (faux_conv_atoul(tmp, &opts->crl_max_size, 10) < 0) {
			lsyslog(LOG_ERR, "Illegal CRLMaxSize: %s\n", tmp);
			faux_ini_free(ini);
			return -1;
		}
		if (opts->crl_max_size > CRSD_DEFAULT_CRL_MAX_SIZE) {
			lsyslog(LOG_ERR, "CRLMaxSize can't be greater than %lu: %s\n",
				CRSD_DEFAULT_CRL_MAX_SIZE, tmp);
			faux_ini_free(ini);
			return -1;
		}
	}

	if ((tmp = faux_ini_find(ini, "CRLStorageMaxSize"))) {
		if (faux_conv_atoul(tmp, &opts->crl_storage_max_size, 10) < 0) {
			lsyslog(LOG_ERR, "Illegal CRLStorageMaxSize: %s\n", tmp);
			faux_ini_free(ini);
			return -1;
		}
	}

	if ((tmp = faux_ini_find(ini, "RootCertDir"))) {
		faux_str_free(opts->rootcert_dir);
		opts->rootcert_dir = faux_str_dup(tmp);
	}

	if ((tmp = faux_ini_find(ini, "CACertDir"))) {
		faux_str_free(opts->cacert_dir);
		opts->cacert_dir = faux_str_dup(tmp);
	}

	if ((tmp = faux_ini_find(ini, "CertDir"))) {
		faux_str_free(opts->cert_dir);
		opts->cert_dir = faux_str_dup(tmp);
	}

	if ((tmp = faux_ini_find(ini, "OCSPCertDir"))) {
		faux_str_free(opts->ocspcert_dir);
		opts->ocspcert_dir = faux_str_dup(tmp);
	}

	if ((tmp = faux_ini_find(ini, "CRLDir"))) {
		faux_str_free(opts->crl_dir);
		opts->crl_dir = faux_str_dup(tmp);
	}

	if ((tmp = faux_ini_find(ini, "KeyDir"))) {
		faux_str_free(opts->key_dir);
		opts->key_dir = faux_str_dup(tmp);
	}

	if ((tmp = faux_ini_find(ini, "P10Dir"))) {
		faux_str_free(opts->p10_dir);
		opts->p10_dir = faux_str_dup(tmp);
	}

	if ((tmp = faux_ini_find(ini, "SecretsFile"))) {
		faux_str_free(opts->secrets_file);
		opts->secrets_file = faux_str_dup(tmp);
	}

	faux_ini_free(ini);
	return 0;
}


/** @brief Show options. For debug purposes.
 */
static int opts_show(struct options *opts)
{
	assert(opts);
	if (!opts)
		return -1;

	lsyslog(LOG_DEBUG, "opts: Foreground = %s\n", opts->foreground ? "true" : "false");
	lsyslog(LOG_DEBUG, "opts: Verbose = %s\n", opts->verbose ? "true" : "false");
	lsyslog(LOG_DEBUG, "opts: LogFacility = %s\n", faux_log_facility_str(opts->log_facility));
	lsyslog(LOG_DEBUG, "opts: PIDPath = %s\n", opts->pidfile);
	lsyslog(LOG_DEBUG, "opts: ConfigPath = %s\n", opts->cfgfile);
	lsyslog(LOG_DEBUG, "opts: UnixSocketPath = %s\n", opts->unix_socket_path);
	lsyslog(LOG_DEBUG, "opts: UID = %s\n", opts->uid_str ? opts->uid_str : "");
	lsyslog(LOG_DEBUG, "opts: GID = %s\n", opts->gid_str ? opts->gid_str : "");
	lsyslog(LOG_DEBUG, "opts: FetchInterval = %ld sec\n", opts->fetch_interval);
	lsyslog(LOG_DEBUG, "opts: FetchMargin = %u %% \n", opts->fetch_margin);
	lsyslog(LOG_DEBUG, "opts: FetchTimeout = %ld sec\n", opts->fetch_timeout);
	lsyslog(LOG_DEBUG, "opts: CRLMaxSize = %ld bytes\n", opts->crl_max_size);
	lsyslog(LOG_DEBUG, "opts: CRLStorageMaxSize = %ld bytes\n", opts->crl_storage_max_size);
	lsyslog(LOG_DEBUG, "opts: RootCertDir = %s\n", opts->rootcert_dir);
	lsyslog(LOG_DEBUG, "opts: CACertDir = %s\n", opts->cacert_dir);
	lsyslog(LOG_DEBUG, "opts: CertDir = %s\n", opts->cert_dir);
	lsyslog(LOG_DEBUG, "opts: OCSPCertDir = %s\n", opts->ocspcert_dir);
	lsyslog(LOG_DEBUG, "opts: CRLDir = %s\n", opts->crl_dir);
	lsyslog(LOG_DEBUG, "opts: KeyDir = %s\n", opts->key_dir);
	lsyslog(LOG_DEBUG, "opts: P10Dir = %s\n", opts->p10_dir);
	lsyslog(LOG_DEBUG, "opts: SecretsFile = %s\n", opts->secrets_file);

	return 0;
}
