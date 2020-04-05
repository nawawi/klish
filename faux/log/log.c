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

	if (!faux_str_casecmp(str, "local0"))
		*facility = LOG_LOCAL0;
	else if (!faux_str_casecmp(str, "local1"))
		*facility = LOG_LOCAL1;
	else if (!faux_str_casecmp(str, "local2"))
		*facility = LOG_LOCAL2;
	else if (!faux_str_casecmp(str, "local3"))
		*facility = LOG_LOCAL3;
	else if (!faux_str_casecmp(str, "local4"))
		*facility = LOG_LOCAL4;
	else if (!faux_str_casecmp(str, "local5"))
		*facility = LOG_LOCAL5;
	else if (!faux_str_casecmp(str, "local6"))
		*facility = LOG_LOCAL6;
	else if (!faux_str_casecmp(str, "local7"))
		*facility = LOG_LOCAL7;
	else if (!faux_str_casecmp(str, "auth"))
		*facility = LOG_AUTH;
#ifdef LOG_AUTHPRIV
	else if (!faux_str_casecmp(str, "authpriv"))
		*facility = LOG_AUTHPRIV;
#endif
	else if (!faux_str_casecmp(str, "cron"))
		*facility = LOG_CRON;
	else if (!faux_str_casecmp(str, "daemon"))
		*facility = LOG_DAEMON;
#ifdef LOG_FTP
	else if (!faux_str_casecmp(str, "ftp"))
		*facility = LOG_FTP;
#endif
	else if (!faux_str_casecmp(str, "kern"))
		*facility = LOG_KERN;
	else if (!faux_str_casecmp(str, "lpr"))
		*facility = LOG_LPR;
	else if (!faux_str_casecmp(str, "mail"))
		*facility = LOG_MAIL;
	else if (!faux_str_casecmp(str, "news"))
		*facility = LOG_NEWS;
	else if (!faux_str_casecmp(str, "syslog"))
		*facility = LOG_SYSLOG;
	else if (!faux_str_casecmp(str, "user"))
		*facility = LOG_USER;
	else if (!faux_str_casecmp(str, "uucp"))
		*facility = LOG_UUCP;
	else
		return -1;

	return 0;
}
