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
#include <klish/ksession.h>
#include <klish/kpath.h>


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
		fprintf(stderr, "Empty item\n");
		return -1;
	}

	printf("[%s]\n", script);

	return 0;
}


// Print content of ACTION script
int klish_printl(kcontext_t *context)
{
	const kaction_t *action = NULL;
	const char *script = NULL;

	action = (kaction_t *)faux_list_data(kcontext_action_iter(context));

	script = kaction_script(action);
	if (faux_str_is_empty(script))
		script = "";

	printf("%s\n", script);

	return 0;
}


// Print content of ACTION script. Without additional '/n'
int klish_print(kcontext_t *context)
{
	const kaction_t *action = NULL;
	const char *script = NULL;

	action = (kaction_t *)faux_list_data(kcontext_action_iter(context));

	script = kaction_script(action);
	if (faux_str_is_empty(script))
		script = "";

	printf("%s", script);
	fflush(stdout);

	return 0;
}


// Symbol to show current path
int klish_pwd(kcontext_t *context)
{
	kpath_t *path = NULL;
	kpath_levels_node_t *iter = NULL;
	klevel_t *level = NULL;

	path = ksession_path(kcontext_session(context));
	iter = kpath_iter(path);
	while ((level = kpath_each(&iter))) {
		printf("/%s", kentry_name(klevel_entry(level)));
	}
	printf("\n");

	return 0;
}
