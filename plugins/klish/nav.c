/** @file nav.c
 * @brief Navigation
 *
 * Example:
 * <ACTION sym="nav">
 *     pop
 *     push /view_name
 * </ACTION>
 *
 * Possible navigation commands:
 * * push <view_name> - Push "view_name" view to new level of path and change
 *     current path to it.
 * * pop [num] - Pop up path levels. Optional "num" argument specifies number
 *     of levels to pop. Default is 1.
 * * top - Pop up to first path level.
 * * replace <view_name> - Replace current view by the specified one.
 * * exit - Exit klish.
 */

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <faux/str.h>
#include <faux/list.h>
#include <faux/conv.h>
#include <klish/kcontext.h>
#include <klish/kentry.h>
#include <klish/ksession.h>
#include <klish/kpath.h>
#include <klish/kscheme.h>


int klish_nav(kcontext_t *context)
{
	const char *script = NULL;
	ksession_t *session = NULL;
	kpath_t *path = NULL;
	const char *str = NULL;
	char *line = NULL;

	// Navigation is suitable only for command actions but not for
	// PTYPEs, CONDitions i.e. SERVICE_ACTIONS.
	assert(kcontext_type(context) == KCONTEXT_ACTION);

	script = kcontext_script(context);
	if (faux_str_is_empty(script)) // No navigation commands. It's not an error.
		return 0;
	session = kcontext_session(context);
	assert(session);
	path = ksession_path(session);

	// Iterate lines from "script". Each line is navigation command.
	str = script;
	while ((line = faux_str_getline(str, &str))) {
		faux_argv_t *argv = faux_argv_new();
		ssize_t lnum = faux_argv_parse(argv, line);
		const char *nav_cmd = NULL;

		faux_str_free(line);
		if (lnum < 1) {
			faux_argv_free(argv);
			continue;
		}
		nav_cmd = faux_argv_index(argv, 0);

		// exit
		if (faux_str_casecmp(nav_cmd, "exit") == 0) {
			ksession_set_done(session, BOOL_TRUE);
			// "Exit" supposes another navigation commands have no
			// meaning. So break the loop.
			faux_argv_free(argv);
			break;

		// top
		} else if (faux_str_casecmp(nav_cmd, "top") == 0) {
			while (kpath_len(path) > 1) {
				if (!kpath_pop(path)) {
					faux_argv_free(argv);
					return -1;
				}
			}

		// pop [number_of_levels]
		} else if (faux_str_casecmp(nav_cmd, "pop") == 0) {
			size_t i = 0;
			size_t lnum = 1; // Default levels to pop
			const char *lnum_str = faux_argv_index(argv, 1);
			// Level number is specified
			if (lnum_str) {
				unsigned char val = 0;
				// 8 bit unsigned integer is enough
				if (!faux_conv_atouc(lnum_str, &val, 0)) {
					faux_argv_free(argv);
					return -1;
				}
				lnum = val;
			}
			// Don't pop upper than top level
			// Such "pop" means exit
			if (lnum > (kpath_len(path) - 1)) {
				ksession_set_done(session, BOOL_TRUE);
				faux_argv_free(argv);
				break;
			}
			// Pop levels
			for (i = 0; i < lnum; i++) {
				if (!kpath_pop(path)) {
					faux_argv_free(argv);
					return -1;
				}
			}

		// push <view_name>
		} else if (faux_str_casecmp(nav_cmd, "push") == 0) {
			const char *view_name = faux_argv_index(argv, 1);
			kentry_t *new_view = NULL;
			if (!view_name) {
				faux_argv_free(argv);
				return -1;
			}
			new_view = kscheme_find_entry_by_path(
				ksession_scheme(session), view_name);
			if (!new_view) {
				faux_argv_free(argv);
				return -1;
			}
			if (!kpath_push(path, klevel_new(new_view))) {
				faux_argv_free(argv);
				return -1;
			}

		// replace <view_name>
		} else if (faux_str_casecmp(nav_cmd, "replace") == 0) {
			const char *view_name = faux_argv_index(argv, 1);
			kentry_t *new_view = NULL;
			if (!view_name) {
				faux_argv_free(argv);
				return -1;
			}
			new_view = kscheme_find_entry_by_path(
				ksession_scheme(session), view_name);
			if (!new_view) {
				faux_argv_free(argv);
				return -1;
			}
			if (!kpath_pop(path)) {
				faux_argv_free(argv);
				return -1;
			}
			if (!kpath_push(path, klevel_new(new_view))) {
				faux_argv_free(argv);
				return -1;
			}

		// Unknown command
		} else {
			faux_argv_free(argv);
			return -1;
		}

		faux_argv_free(argv);
	}

	return 0;
}
