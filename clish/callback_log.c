/*
 * callback_log.c
 *
 * Callback hook to log users's commands
 */
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

#include "internal.h"

#define SYSLOG_IDENT "klish"
#define SYSLOG_FACILITY LOG_LOCAL0

/*--------------------------------------------------------- */
int clish_log_callback(clish_context_t *context, const char *line,
	int retcode)
{
	/* Initialization */
	if (!line) {
		openlog(SYSLOG_IDENT, LOG_PID, SYSLOG_FACILITY);
		return 0;
	}

	/* Log the given line */
	syslog(LOG_INFO, "%s : %d", line, retcode);

	return 0;
}
