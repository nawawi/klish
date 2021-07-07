
#ifndef VERSION
#define VERSION "1.0.0"
#endif


/** @brief Command line and config file options
 */
struct options {
	bool_t verbose;
	char *unix_socket_path;
	char *line;
};

// Options
void help(int status, const char *argv0);
struct options *opts_init(void);
void opts_free(struct options *opts);
int opts_parse(int argc, char *argv[], struct options *opts);
