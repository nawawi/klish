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
#include <klish/kexec.h>
#include <klish/kpath.h>


int klish_syslog(kcontext_t *context)
{
	const kcontext_t *parent_context = NULL;
	const ksession_t *session = NULL;
	const kexec_t *parent_exec = NULL;
	char *log_full_line = NULL;

	assert(context);
	parent_context = kcontext_parent_context(context);
	if (!parent_context)
		return -1;
	session = kcontext_session(context);
	if (!session)
		return -1;
	parent_exec = kcontext_parent_exec(context);
	if (parent_exec && (kexec_contexts_len(parent_exec) > 1)) {
		log_full_line = faux_str_sprintf(", (%s)#%u",
			kexec_line(parent_exec),
			kcontext_pipeline_stage(parent_context));
	}

	syslog(LOG_INFO, "%u(%s) %s : %d%s",
		ksession_uid(session), ksession_user(session),
		kcontext_line(parent_context), kcontext_retcode(parent_context),
		log_full_line ? log_full_line : "");

	faux_str_free(log_full_line);

	return 0;
}
