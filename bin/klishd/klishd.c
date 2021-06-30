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
#include <klish/kdb.h>
#include <klish/kpargv.h>

#include "private.h"

#include "sch.c"

// Local static functions
static int create_listen_unix_sock(const char *path);
static bool_t load_all_dbs(kscheme_t *scheme, const char *dbs,
	faux_ini_t *global_config, faux_error_t *error);


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
	ksession_t *session = NULL;
	faux_error_t *error = faux_error_new();
	faux_ini_t *config = NULL;

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

	// Scheme
	scheme = kscheme_new();
	{
	kcontext_t *context = NULL;
	bool_t prepare_retcode = BOOL_FALSE;
	kdb_t *deploy_db = NULL;

	// Load scheme
	if (!load_all_dbs(scheme, opts->dbs, config, error)) {
		fprintf(stderr, "Scheme errors:\n");
		goto err;
	}

	// Prepare scheme
	context = kcontext_new(KCONTEXT_PLUGIN_INIT);
	prepare_retcode = kscheme_prepare(scheme, context, error);
	kcontext_free(context);
	if (!prepare_retcode) {
		fprintf(stderr, "Scheme preparing errors:\n");
		goto err;
	}

	// Deploy (for testing purposes)
	deploy_db = kdb_new("ischeme", NULL);
	kdb_load_plugin(deploy_db);
	kdb_init(deploy_db);
//	kdb_deploy_scheme(deploy_db, scheme);
	kdb_fini(deploy_db);
	kdb_free(deploy_db);
	}

	// Parsing
	{
	char *s = NULL;
	const char *line = "cmd o1 m7 o2 e1";
	kpargv_t *pargv = NULL;
	kpargv_pargs_node_t *p_iter = NULL;
	
	session = ksession_new(scheme, "/lowview");
	kpath_push(ksession_path(session), klevel_new(kscheme_find_entry_by_path(scheme, "/main")));
	pargv = ksession_parse_line(session, line);
	switch (kpargv_status(pargv)) {
	case KPARSE_NONE:
		s = "None";
		break;
	case KPARSE_OK:
		s = "Ok";
		break;
	case KPARSE_INPROGRESS:
		s = "In progress";
		break;
	case KPARSE_NOTFOUND:
		s = "Not found";
		break;
	case KPARSE_INCOMPLETED:
		s = "Incompleted";
		break;
	case KPARSE_ILLEGAL:
		s = "Illegal";
		break;
	case KPARSE_ERROR:
		s = "Error";
		break;
	}
	printf("Level: %lu, Line '%s': %s\n", kpargv_level(pargv), line, s);

	if (pargv) {
		kparg_t *parg = NULL;
		p_iter = kpargv_pargs_iter(pargv);
		while ((parg = kpargv_pargs_each(&p_iter))) {
			printf("%s(%s) ", kparg_value(parg), kentry_name(kparg_entry(parg)));
		}
		printf("\n");
	}
	
	kpargv_free(pargv);
	ksession_free(session);
	
	}

goto err; // Test purposes

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
	if (scheme) {
		kcontext_t *context = kcontext_new(KCONTEXT_PLUGIN_FINI);
		kscheme_fini(scheme, context, error);
		kcontext_free(context);
		kscheme_free(scheme);
	}

	// Free command line options
	opts_free(opts);
	faux_ini_free(config);

	syslog(LOG_INFO, "Stop daemon.\n");

	if (faux_error_len(error) > 0)
		faux_error_show(error);
	faux_error_free(error);

	return retval;
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
	if (!db)
		return BOOL_FALSE;
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


static bool_t load_all_dbs(kscheme_t *scheme, const char *dbs,
	faux_ini_t *global_config, faux_error_t *error)
{
	faux_argv_t *dbs_argv = NULL;
	faux_argv_node_t *iter = NULL;
	const char *db_name = NULL;
	bool_t retcode = BOOL_TRUE;

	assert(scheme);
	if (!scheme)
		return BOOL_FALSE;
	assert(dbs);
	if (!dbs)
		return BOOL_FALSE;

	dbs_argv = faux_argv_new();
	assert(dbs_argv);
	if (!dbs_argv)
		return BOOL_FALSE;
	if (faux_argv_parse(dbs_argv, dbs) <= 0) {
		faux_argv_free(dbs_argv);
		return BOOL_FALSE;
	}

	// For each DB
	iter = faux_argv_iter(dbs_argv);
	while ((db_name = faux_argv_each(&iter))) {
		faux_ini_t *config = NULL; // Sub-config for current DB
		char *prefix = NULL;

		prefix = faux_str_mcat(&prefix, "DB.", db_name, ".", NULL);
		if (config)
			config = faux_ini_extract_subini(global_config, prefix);
		if (!load_db(scheme, db_name, config, error))
			retcode = BOOL_FALSE;
		faux_ini_free(config);
		faux_str_free(prefix);
	}

	faux_argv_free(dbs_argv);

	return retcode;
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
