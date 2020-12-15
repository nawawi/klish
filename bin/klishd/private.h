#ifndef VERSION
#define VERSION "1.0.0"
#endif

#define LOG_NAME "klishd"
#define DEFAULT_PIDFILE "/var/run/klishd.pid"
#define DEFAULT_CFGFILE "/etc/klish/klishd.conf"


/** @brief Command line and config file options
 */
struct options {
	char *pidfile;
	char *cfgfile;
	char *unix_socket_path;
	bool_t cfgfile_userdefined;
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
int config_parse(const char *cfgfile, struct options *opts);
