/** @file log.c
 * @brief Helpers for logging
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <syslog.h>

#include "faux/str.h"
#include "faux/log.h"


/** @brief Parses syslog facility string and returns the facility id.
 *
 * Gets syslog facility string, parse it and finds out the facility
 * id in digital form. Usefull config or command line options parsing.
 *
 * @param [in] str Facility string.
 * @param [out] facility Facility in digital form.
 * @returns 0 - success, < 0 - parsing error
 */
int faux_log_facility(const char *str, int *facility) {

	if (faux_str_casecmp(str, "local0") == 0)
		*facility = LOG_LOCAL0;
	else if (faux_str_casecmp(str, "local1") == 0)
		*facility = LOG_LOCAL1;
	else if (faux_str_casecmp(str, "local2") == 0)
		*facility = LOG_LOCAL2;
	else if (faux_str_casecmp(str, "local3") == 0)
		*facility = LOG_LOCAL3;
	else if (faux_str_casecmp(str, "local4") == 0)
		*facility = LOG_LOCAL4;
	else if (faux_str_casecmp(str, "local5") == 0)
		*facility = LOG_LOCAL5;
	else if (faux_str_casecmp(str, "local6") == 0)
		*facility = LOG_LOCAL6;
	else if (faux_str_casecmp(str, "local7") == 0)
		*facility = LOG_LOCAL7;
	else if (faux_str_casecmp(str, "auth") == 0)
		*facility = LOG_AUTH;
#ifdef LOG_AUTHPRIV
	else if (faux_str_casecmp(str, "authpriv") == 0)
		*facility = LOG_AUTHPRIV;
#endif
	else if (faux_str_casecmp(str, "cron") == 0)
		*facility = LOG_CRON;
	else if (faux_str_casecmp(str, "daemon") == 0)
		*facility = LOG_DAEMON;
#ifdef LOG_FTP
	else if (faux_str_casecmp(str, "ftp") == 0)
		*facility = LOG_FTP;
#endif
	else if (faux_str_casecmp(str, "kern") == 0)
		*facility = LOG_KERN;
	else if (faux_str_casecmp(str, "lpr") == 0)
		*facility = LOG_LPR;
	else if (faux_str_casecmp(str, "mail") == 0)
		*facility = LOG_MAIL;
	else if (faux_str_casecmp(str, "news") == 0)
		*facility = LOG_NEWS;
	else if (faux_str_casecmp(str, "syslog") == 0)
		*facility = LOG_SYSLOG;
	else if (faux_str_casecmp(str, "user") == 0)
		*facility = LOG_USER;
	else if (faux_str_casecmp(str, "uucp") == 0)
		*facility = LOG_UUCP;
	else
		return -1;

	return 0;
}
