#ifndef VERSION
#define VERSION "1.0.0"
#endif

#include <faux/ini.h>

#define LOG_NAME "klishd-listen"
#define LOG_SERVICE_NAME "klishd"
#define DEFAULT_PIDFILE "/var/run/klishd.pid"
#define DEFAULT_CFGFILE "/etc/klish/klishd.conf"
#define DEFAULT_DBS "libxml2"


/** @brief Command line and config file options
 */
struct options {
	char *pidfile;
	char *cfgfile;
	bool_t cfgfile_userdefined;
	char *unix_socket_path;
	char *dbs;
	bool_t foreground; // Don't daemonize
	bool_t verbose;
	int log_facility;
};

// Options and config file
void help(int status, const char *argv0);
struct options *opts_init(void);
void opts_free(struct options *opts);
int opts_parse(int argc, char *argv[], struct options *opts);
int opts_show(struct options *opts);
faux_ini_t *config_parse(const char *cfgfile, struct options *opts);
