#include <faux/list.h>
#include <klish/ktp.h>
#include <klish/ktp_session.h>

#ifndef VERSION
#define VERSION "1.0.0"
#endif

#define DEFAULT_CFGFILE "/etc/klish/klish.conf"
#define DEFAULT_PAGER "/usr/bin/less -I -F -e -X -K -d"

/** @brief Command line and config file options
 */
struct options {
	bool_t verbose;
	char *cfgfile;
	bool_t cfgfile_userdefined;
	char *unix_socket_path;
	char *pager;
	bool_t pager_enabled;
	bool_t stop_on_error;
	bool_t dry_run;
	bool_t quiet;
	faux_list_t *commands;
	faux_list_t *files;
};

// Options
void help(int status, const char *argv0);
struct options *opts_init(void);
void opts_free(struct options *opts);
int opts_parse(int argc, char *argv[], struct options *opts);
bool_t config_parse(const char *cfgfile, struct options *opts);

// Interactive shell
int klish_interactive_shell(ktp_session_t *ktp, struct options *opts);
