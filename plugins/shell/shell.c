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

#include <faux/str.h>
#include <faux/list.h>
#include <klish/kcontext.h>
#include <klish/ksession.h>


// Execute shell script
int shell_shell(kcontext_t *context)
{
	const char *script = NULL;

	script = kcontext_script(context);
	if (faux_str_is_empty(script))
		return 0;

//	printf("%s", prompt);
//	faux_str_free(prompt);
//	fflush(stdout);

	return 0;
}
