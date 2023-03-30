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
#include <faux/argv.h>
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
#include <klish/ischeme.h>
#include <klish/kcontext.h>
#include <klish/ksession.h>
#include <klish/ksession_parse.h>
#include <klish/kdb.h>
#include <klish/kpargv.h>

#include "private.h"


// Local static functions
bool_t daemonize(const char *pidfile);
bool_t kentry_entrys_is_empty(const kentry_t *entry);
static int create_listen_unix_sock(const char *path);
static kscheme_t *load_all_dbs(const char *dbs,
	faux_ini_t *global_config, faux_error_t *error);
static bool_t clear_scheme(kscheme_t *scheme, faux_error_t *error);
static void signal_handler_empty(int signo);


// Main loop events
static bool_t stop_loop_ev(faux_eloop_t *eloop, faux_eloop_type_e type,
	void *associated_data, void *user_data);
static bool_t refresh_config_ev(faux_eloop_t *eloop, faux_eloop_type_e type,
	void *associated_data, void *user_data);
static bool_t listen_socket_ev(faux_eloop_t *eloop, faux_eloop_type_e type,
	void *associated_data, void *user_data);
static bool_t wait_for_child_ev(faux_eloop_t *eloop, faux_eloop_type_e type,
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
	ktpd_session_t *ktpd_session = NULL;
	kscheme_t *scheme = NULL;
	faux_error_t *error = faux_error_new();
	faux_ini_t *config = NULL;
	int client_fd = -1;
	struct sigaction sig_act = {};
	sigset_t sig_set = {};

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
		if (!(config = config_parse(opts->cfgfile, opts)))
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

	// Fork the daemon if needed
	if (!opts->foreground && !daemonize(opts->pidfile))
			goto err;

	// Load scheme
	if (!(scheme = load_all_dbs(opts->dbs, config, error))) {
		fprintf(stderr, "Scheme errors:\n");
		goto err;
	}

	// Listen socket
	syslog(LOG_DEBUG, "Create listen UNIX socket: %s\n", opts->unix_socket_path);
	listen_unix_sock = create_listen_unix_sock(opts->unix_socket_path);
	if (listen_unix_sock < 0)
		goto err;
	syslog(LOG_DEBUG, "Listen socket %d", listen_unix_sock);

	// Event loop
	eloop = faux_eloop_new(NULL);
	// Signals
	faux_eloop_add_signal(eloop, SIGINT, stop_loop_ev, NULL);
	faux_eloop_add_signal(eloop, SIGTERM, stop_loop_ev, NULL);
	faux_eloop_add_signal(eloop, SIGQUIT, stop_loop_ev, NULL);
	faux_eloop_add_signal(eloop, SIGHUP, refresh_config_ev, opts);
	faux_eloop_add_signal(eloop, SIGCHLD, wait_for_child_ev, NULL);
	// Listen socket. Waiting for new connections
	faux_eloop_add_fd(eloop, listen_unix_sock, POLLIN,
		listen_socket_ev, &client_fd);
	// Scheduled events
//	faux_eloop_add_sched_once_delayed(eloop, &delayed, 1, sched_once, NULL);
//	faux_eloop_add_sched_periodic_delayed(eloop, 2, sched_periodic, NULL, &period, FAUX_SCHED_INFINITE);
	// Main loop
	faux_eloop_loop(eloop);
	faux_eloop_free(eloop);

	retval = 0;

err: // For listen daemon
	// Print errors
	if (faux_error_len(error) > 0)
		faux_error_show(error);
	faux_error_free(error);

	// Close listen socket
	if (listen_unix_sock >= 0)
		close(listen_unix_sock);

	// Finish listen daemon if it's not forked service process.
	if (client_fd < 0) {

		// Free scheme
		clear_scheme(scheme, error);

		// Free command line options
		opts_free(opts);
		faux_ini_free(config);

		// Remove pidfile
		if (pidfd >= 0) {
			if (unlink(opts->pidfile) < 0) {
				syslog(LOG_ERR, "Can't remove pid-file %s: %s\n",
				opts->pidfile, strerror(errno));
			}
		}

		syslog(LOG_INFO, "Stop daemon.\n");

		return retval;
	}

	// ATTENTION: It's a forked service process
	retval = -1; // Pessimism for service process
	eloop = NULL;

	// Re-Initialize syslog
	openlog(LOG_SERVICE_NAME, logoptions, opts->log_facility);
	if (!opts->verbose)
		setlogmask(LOG_UPTO(LOG_INFO));

	// Create event loop
	eloop = faux_eloop_new(NULL);

	// Create KTP session
	// Function ktpd_session_new() will add new events to eloop itself.
	ktpd_session = ktpd_session_new(client_fd, scheme, NULL, eloop);
	assert(ktpd_session);
	if (!ktpd_session) {
		syslog(LOG_ERR, "Can't create KTPd session\n");
		goto err_client;
	}

	syslog(LOG_DEBUG, "New connection %d\n", client_fd);

	// Signals
	faux_eloop_add_signal(eloop, SIGINT, stop_loop_ev, NULL);
	faux_eloop_add_signal(eloop, SIGTERM, stop_loop_ev, NULL);
	faux_eloop_add_signal(eloop, SIGQUIT, stop_loop_ev, NULL);

	// Ignore SIGPIPE from client. Don't use SIG_IGN because it will be
	// inherited.
	sigemptyset(&sig_set);
	sig_act.sa_flags = 0;
	sig_act.sa_mask = sig_set;
	sig_act.sa_handler = &signal_handler_empty;
	sigaction(SIGPIPE, &sig_act, NULL);

	// Main service loop
	faux_eloop_loop(eloop);

	retval = 0;
err_client:

	ktpd_session_free(ktpd_session);
	faux_eloop_free(eloop);
	syslog(LOG_DEBUG, "Close connection %d", client_fd);
	close(client_fd);

	// Free scheme
	clear_scheme(scheme, error);

	// Free command line options
	opts_free(opts);
	faux_ini_free(config);

	return retval;
}


bool_t daemonize(const char *pidfile)
{
	// Daemonize
	syslog(LOG_DEBUG, "Daemonize\n");
	if (!faux_daemon(0, 0, pidfile, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) {
		syslog(LOG_ERR, "Can't daemonize\n");
		return BOOL_FALSE;
	}

	syslog(LOG_DEBUG, "PID file: %s\n", pidfile);

	return BOOL_TRUE;
}


static bool_t load_db(kscheme_t *scheme, const char *db_name,
	faux_ini_t *config, faux_error_t *error)
{
	kdb_t *db = NULL;
	const char *sofile = NULL;

	assert(scheme);
	if (!scheme)
		return BOOL_FALSE;
	assert(db_name);
	if (!db_name)
		return BOOL_FALSE;

	// DB.libxml2.so = <so filename>
	if (config)
		sofile = faux_ini_find(config, "so");

	db = kdb_new(db_name, sofile);
	assert(db);
	if (!db) {
		faux_ini_free(config);
		return BOOL_FALSE;
	}
	// Now kdb owns config
	kdb_set_ini(db, config);
	kdb_set_error(db, error);

	// Load DB plugin
	if (!kdb_load_plugin(db)) {
		faux_error_sprintf(error,
			"DB \"%s\": Can't load DB plugin", db_name);
		kdb_free(db);
		return BOOL_FALSE;
	}

	// Check plugin API version
	if ((kdb_major(db) != KDB_MAJOR) ||
		(kdb_minor(db) != KDB_MINOR)) {
		faux_error_sprintf(error,
			"DB \"%s\": Plugin's API version is %u.%u, need %u.%u",
			db_name,
			kdb_major(db), kdb_minor(db),
			KDB_MAJOR, KDB_MINOR);
		kdb_free(db);
		return BOOL_FALSE;
	}

	// Init plugin
	if (kdb_has_init_fn(db) && !kdb_init(db)) {
		faux_error_sprintf(error,
			"DB \"%s\": Can't init DB plugin", db_name);
		kdb_free(db);
		return BOOL_FALSE;
	}

	// Load scheme
	if (!kdb_has_load_fn(db) || !kdb_load_scheme(db, scheme)) {
		faux_error_sprintf(error,
			"DB \"%s\": Can't load scheme from DB plugin", db_name);
		kdb_fini(db);
		kdb_free(db);
		return BOOL_FALSE;
	}

	// Fini plugin
	if (kdb_has_fini_fn(db) && !kdb_fini(db)) {
		faux_error_sprintf(error,
			"DB \"%s\": Can't fini DB plugin", db_name);
		kdb_free(db);
		return BOOL_FALSE;
	}

	kdb_free(db);

	return BOOL_TRUE;
}


static kscheme_t *load_all_dbs(const char *dbs,
	faux_ini_t *global_config, faux_error_t *error)
{
	kscheme_t *scheme = NULL;
	faux_argv_t *dbs_argv = NULL;
	faux_argv_node_t *iter = NULL;
	const char *db_name = NULL;
	bool_t retcode = BOOL_TRUE;
	kcontext_t *context = NULL;

	assert(dbs);
	if (!dbs)
		return NULL;

	scheme = kscheme_new();
	assert(scheme);
	if (!scheme)
		return NULL;

	dbs_argv = faux_argv_new();
	assert(dbs_argv);
	if (!dbs_argv) {
		kscheme_free(scheme);
		return NULL;
	}
	if (faux_argv_parse(dbs_argv, dbs) <= 0) {
		kscheme_free(scheme);
		faux_argv_free(dbs_argv);
		return NULL;
	}

	// For each DB
	iter = faux_argv_iter(dbs_argv);
	while ((db_name = faux_argv_each(&iter))) {
		faux_ini_t *config = NULL; // Sub-config for current DB
		if (global_config) {
			char *prefix = NULL;
			prefix = faux_str_mcat(&prefix, "DB.", db_name, ".", NULL);
			config = faux_ini_extract_subini(global_config, prefix);
			faux_str_free(prefix);
		}
		if (!load_db(scheme, db_name, config, error))
			retcode = BOOL_FALSE;
	}

	faux_argv_free(dbs_argv);

	// Something went wrong while loading DBs
	if (!retcode) {
		kscheme_free(scheme);
		return NULL;
	}

	// Prepare scheme
	context = kcontext_new(KCONTEXT_TYPE_PLUGIN_INIT);
	kcontext_set_scheme(context, scheme);
	retcode = kscheme_prepare(scheme, context, error);
	kcontext_free(context);
	if (!retcode) {
		kscheme_free(scheme);
		faux_error_sprintf(error, "Scheme preparing errors.\n");
		return NULL;
	}


	// Debug
/*	{
		kdb_t *deploy_db = NULL;

		// Deploy (for testing purposes)
		deploy_db = kdb_new("ischeme", NULL);
		kdb_load_plugin(deploy_db);
		kdb_init(deploy_db);
		kdb_deploy_scheme(deploy_db, scheme);
		kdb_fini(deploy_db);
		kdb_free(deploy_db);
	}
*/

	return scheme;
}


static bool_t clear_scheme(kscheme_t *scheme, faux_error_t *error)
{
	kcontext_t *context = NULL;

	if (!scheme)
		return BOOL_TRUE; // It's not an error

	context = kcontext_new(KCONTEXT_TYPE_PLUGIN_FINI);
	kcontext_set_scheme(context, scheme);
	kscheme_fini(scheme, context, error);
	kcontext_free(context);
	kscheme_free(scheme);

	return BOOL_TRUE;
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


/** @brief Wait for child processes (service processes).
 */
static bool_t wait_for_child_ev(faux_eloop_t *eloop, faux_eloop_type_e type,
	void *associated_data, void *user_data)
{
	int wstatus = 0;
	pid_t child_pid = -1;

	// Wait for any child process. Doesn't block.
	while ((child_pid = waitpid(-1, &wstatus, WNOHANG)) > 0) {
		if (WIFSIGNALED(wstatus)) {
			syslog(LOG_ERR, "Service process %d was terminated "
				"by signal: %d",
				child_pid, WTERMSIG(wstatus));
		} else {
			syslog(LOG_ERR, "Service process %d was terminated: %d",
				child_pid, WEXITSTATUS(wstatus));
		}
	}

	// Happy compiler
	eloop = eloop;
	type = type;
	associated_data = associated_data;
	user_data = user_data;

	return BOOL_TRUE;
}


/** @brief Re-read config file.
 *
 * This function can refresh klishd options but plugins (dbs for example) are
 * already inited and there is no way to re-init them on-the-fly.
 */
static bool_t refresh_config_ev(faux_eloop_t *eloop, faux_eloop_type_e type,
	void *associated_data, void *user_data)
{
	struct options *opts = (struct options *)user_data;
	faux_ini_t *ini = NULL;

	if (access(opts->cfgfile, R_OK) == 0) {
		syslog(LOG_DEBUG, "Re-reading config file \"%s\"\n", opts->cfgfile);
		if (!(ini = config_parse(opts->cfgfile, opts)))
			syslog(LOG_ERR, "Error while config file parsing.\n");
	} else if (opts->cfgfile_userdefined) {
		syslog(LOG_ERR, "Can't find config file \"%s\"\n", opts->cfgfile);
	}
	faux_ini_free(ini); // No way to use it later

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
	pid_t child_pid = -1;

	assert(user_data);

	new_conn = accept(info->fd, NULL, NULL);
	if (new_conn < 0) {
		syslog(LOG_ERR, "Can't accept() new connection");
		return BOOL_TRUE;
	}

	// Fork new instance for newly connected client
	child_pid = fork();
	if (child_pid < 0) {
		close(new_conn);
		syslog(LOG_ERR, "Can't fork service process for client");
		return BOOL_TRUE;
	}

	// Parent
	if (child_pid > 0) {
		close(new_conn); // It's needed by child but not for parent
		syslog(LOG_ERR, "Service process for client was forked: %d",
			child_pid);
		return BOOL_TRUE;
	}

	// Child (forked service process)

	// Pass new ktpd_session to main programm
	*((int *)user_data) = new_conn;

	type = type; // Happy compiler
	eloop = eloop;

	// Return BOOL_FALSE to break listen parent loop. Child will create its
	// own loop then.
	return BOOL_FALSE;
}


static void signal_handler_empty(int signo)
{
	signo = signo; // Happy compiler
}
