/*
 *
 */

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <syslog.h>

#include <faux/str.h>
#include <faux/list.h>
#include <faux/sysdb.h>
#include <klish/kcontext.h>
#include <klish/ksession.h>
#include <klish/kpath.h>


int klish_syslog(kcontext_t *context)
{
	const kcontext_t *parent_context = NULL;
	const ksession_t *session = NULL;

	assert(context);
	parent_context = kcontext_parent_context(context);
	if (!parent_context)
		return -1;
	session = kcontext_session(context);
	if (!session)
		return -1;

	syslog(LOG_INFO, "%u(%s) %s : %d",
		ksession_uid(session), ksession_user(session),
		kcontext_line(parent_context), kcontext_retcode(parent_context));

/*	uname = clish_shell_format_username(this);
	syslog(LOG_INFO, "%u(%s) %s : %d",
		user ? user->pw_uid : getuid(), uname, line, retcode);
	free(uname);
*/

	return 0;
}
