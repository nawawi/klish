/*
 *
 */

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <faux/str.h>
#include <faux/list.h>
#include <klish/kcontext.h>


int klish_nop(kcontext_t *context)
{
	context = context; // Happy compiler

	return 0;
}


// Symbol for testing purposes
int klish_tsym(kcontext_t *context)
{
	const kaction_t *action = NULL;
	const char *script = NULL;

	action = (kaction_t *)faux_list_data(kcontext_action_iter(context));

	script = kaction_script(action);
	if (faux_str_is_empty(script)) {
		printf("[<empty>]\n");
		return -1;
	}

	printf("[%s]\n", script);

	return 0;
}
