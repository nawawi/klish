/*
 * ------------------------------------------------------
 * shell_xml.c
 *
 * This file implements the means to read an XML encoded file and populate the
 * CLI tree based on the contents.
 * ------------------------------------------------------
 */
#include "private.h"
#include "xmlapi.h"
#include "lub/string.h"
#include "lub/ctype.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

typedef void (PROCESS_FN) (clish_shell_t * instance,
	clish_xmlnode_t * element, void *parent);

/* Define a control block for handling the decode of an XML file */
typedef struct clish_xml_cb_s clish_xml_cb_t;
struct clish_xml_cb_s {
	const char *element;
	PROCESS_FN *handler;
};

/* forward declare the handler functions */
static PROCESS_FN
	process_clish_module,
	process_startup,
	process_view,
	process_command,
	process_param,
	process_action,
	process_ptype,
	process_overview,
	process_detail,
	process_namespace,
	process_config,
	process_var,
	process_wdog;

static clish_xml_cb_t xml_elements[] = {
	{"CLISH_MODULE", process_clish_module},
	{"STARTUP", process_startup},
	{"VIEW", process_view},
	{"COMMAND", process_command},
	{"PARAM", process_param},
	{"ACTION", process_action},
	{"PTYPE", process_ptype},
	{"OVERVIEW", process_overview},
	{"DETAIL", process_detail},
	{"NAMESPACE", process_namespace},
	{"CONFIG", process_config},
	{"VAR", process_var},
	{"WATCHDOG", process_wdog},
	{NULL, NULL}
};

/*
 * ------------------------------------------------------
 * This function reads an element from the XML stream and processes it.
 * ------------------------------------------------------
 */
static void process_node(clish_shell_t * shell, clish_xmlnode_t * node, void *parent)
{
	switch (clish_xmlnode_get_type(node)) {
	case CLISH_XMLNODE_ELM: {
			clish_xml_cb_t * cb;
			char name[128];
			unsigned int namelen = sizeof(name);
			if (clish_xmlnode_get_name(node, name, &namelen) == 0) {
				for (cb = &xml_elements[0]; cb->element; cb++) {
					if (0 == strcmp(name, cb->element)) {
#ifdef DEBUG
						fprintf(stderr, "NODE:");
						clish_xmlnode_print(node, stderr);
						fprintf(stderr, "\n");
#endif
						/* process the elements at this level */
						cb->handler(shell, node, parent);
						break;
					}
				}
			}
			break;
		}
	case CLISH_XMLNODE_DOC:
	case CLISH_XMLNODE_TEXT:
	case CLISH_XMLNODE_ATTR:
	case CLISH_XMLNODE_PI:
	case CLISH_XMLNODE_COMMENT:
	case CLISH_XMLNODE_DECL:
	case CLISH_XMLNODE_UNKNOWN:
	default:
		break;
	}
}

/* ------------------------------------------------------ */
static void process_children(clish_shell_t * shell,
	clish_xmlnode_t * element, void *parent)
{
	clish_xmlnode_t *node = NULL;

	while ((node = clish_xmlnode_next_child(element, node)) != NULL) {
		/* Now deal with all the contained elements */
		process_node(shell, node, parent);
	}
}

/* ------------------------------------------------------ */
static void
process_clish_module(clish_shell_t * shell, clish_xmlnode_t * element, void *parent)
{
	// create the global view
	if (!shell->global)
		shell->global = clish_shell_find_create_view(shell,
			"global", "");
	process_children(shell, element, shell->global);
}

/* ------------------------------------------------------ */
static void process_view(clish_shell_t * shell, clish_xmlnode_t * element, void *parent)
{
	clish_view_t *view;
	char name[128] = "";
	char prompt[128] = "";
	char depth[128] = "";
	char restore[128] = "";
	clish_xmlattr_t *a_name;
	clish_xmlattr_t *a_prompt;
	clish_xmlattr_t *a_depth;
	clish_xmlattr_t *a_restore;
	clish_xmlattr_t *a_access;
	int allowed = 1;

	a_name = clish_xmlnode_fetch_attr(element, "name");
	a_prompt = clish_xmlnode_fetch_attr(element, "prompt");
	a_depth = clish_xmlnode_fetch_attr(element, "depth");
	a_restore = clish_xmlnode_fetch_attr(element, "restore");
	a_access = clish_xmlnode_fetch_attr(element, "access");

	/* Check permissions */
	if (a_access) {
		char access[128] = "";

		allowed = 0;
		clish_xmlattr_get_value_noerr(a_access, access, sizeof(access));
		if (shell->client_hooks->access_fn)
			allowed = shell->client_hooks->access_fn(shell, access);
	}
	if (!allowed)
		return;

	clish_xmlattr_get_value_noerr(a_name, name, sizeof(name));
	clish_xmlattr_get_value_noerr(a_prompt, prompt, sizeof(prompt));

	/* re-use a view if it already exists */
	view = clish_shell_find_create_view(shell, name, prompt);

	clish_xmlattr_get_value_noerr(a_depth, depth, sizeof(depth));

	if (*depth && (lub_ctype_isdigit(*depth))) {
		unsigned res = atoi(depth);
		clish_view__set_depth(view, res);
	}

	clish_xmlattr_get_value_noerr(a_restore, restore, sizeof(restore));

	if (*restore) {
		if (!lub_string_nocasecmp(restore, "depth"))
			clish_view__set_restore(view, CLISH_RESTORE_DEPTH);
		else if (!lub_string_nocasecmp(restore, "view"))
			clish_view__set_restore(view, CLISH_RESTORE_VIEW);
		else
			clish_view__set_restore(view, CLISH_RESTORE_NONE);
	}

	process_children(shell, element, view);
}

/* ------------------------------------------------------ */
static void process_ptype(clish_shell_t * shell, clish_xmlnode_t * element, void *parent)
{
	clish_ptype_method_e method;
	clish_ptype_preprocess_e preprocess;
	clish_ptype_t *ptype;

	clish_xmlattr_t *a_name = clish_xmlnode_fetch_attr(element, "name");
	clish_xmlattr_t *a_help = clish_xmlnode_fetch_attr(element, "help");
	clish_xmlattr_t *a_pattern = clish_xmlnode_fetch_attr(element, "pattern");
	clish_xmlattr_t *a_method_name = clish_xmlnode_fetch_attr(element, "method");
	clish_xmlattr_t *a_preprocess_name =
		clish_xmlnode_fetch_attr(element, "preprocess");

	char name[128] = "";
	char help[128] = "";
	char pattern[128] = "";
	char method_name[128] = "";
	char preprocess_name[128] = "";

	clish_xmlattr_get_value_noerr(a_name, name, sizeof(name));
	clish_xmlattr_get_value_noerr(a_pattern, pattern, sizeof(pattern));

	assert(*name);
	assert(*pattern);

	clish_xmlattr_get_value_noerr(a_method_name, method_name,
		sizeof(method_name));

	method = clish_ptype_method_resolve(method_name);

	clish_xmlattr_get_value_noerr(a_preprocess_name, preprocess_name,
		sizeof(preprocess_name));
	clish_xmlattr_get_value_noerr(a_help, help, sizeof(help));

	preprocess = clish_ptype_preprocess_resolve(preprocess_name);
	ptype = clish_shell_find_create_ptype(shell,
		name, help, pattern, method, preprocess);

	assert(ptype);
}

/* ------------------------------------------------------ */
static void
process_overview(clish_shell_t * shell, clish_xmlnode_t * element, void *parent)
{
	char *content = NULL;
	unsigned int content_len = 2048;
	int result;

	/*
	 * the code below faithfully assume that we'll be able fully store
	 * the content of the node. If it's really, really big, we may have
	 * an issue (but then, if it's that big, how the hell does it
	 * already fits in allocated memory?)
	 * Ergo, it -should- be safe.
	 */
	do {
		content = (char*)realloc(content, content_len);
		result = clish_xmlnode_get_content(element, content,
			&content_len);
	} while (result == -E2BIG);

	if (result == 0 && content) {
		/* set the overview text for this view */
		assert(NULL == shell->overview);
		/* store the overview */
		shell->overview = lub_string_dup(content);
	}

	if (content)
		free(content);
}

/* ------------------------------------------------------ */
static void
process_command(clish_shell_t * shell, clish_xmlnode_t * element, void *parent)
{
	clish_view_t *v = (clish_view_t *) parent;
	clish_command_t *cmd = NULL;
	clish_command_t *old;
	char *alias_name = NULL;
	clish_view_t *alias_view = NULL;
	int allowed = 1;

	clish_xmlattr_t *a_access = clish_xmlnode_fetch_attr(element, "access");
	clish_xmlattr_t *a_name = clish_xmlnode_fetch_attr(element, "name");
	clish_xmlattr_t *a_help = clish_xmlnode_fetch_attr(element, "help");
	clish_xmlattr_t *a_view = clish_xmlnode_fetch_attr(element, "view");
	clish_xmlattr_t *a_viewid = clish_xmlnode_fetch_attr(element, "viewid");
	clish_xmlattr_t *a_escape_chars = clish_xmlnode_fetch_attr(element, "escape_chars");
	clish_xmlattr_t *a_args_name = clish_xmlnode_fetch_attr(element, "args");
	clish_xmlattr_t *a_args_help = clish_xmlnode_fetch_attr(element, "args_help");
	clish_xmlattr_t *a_lock = clish_xmlnode_fetch_attr(element, "lock");
	clish_xmlattr_t *a_interrupt = clish_xmlnode_fetch_attr(element, "interrupt");
	clish_xmlattr_t *a_ref = clish_xmlnode_fetch_attr(element, "ref");

	char name[128];
	char help[128];
	char ref[128];
	char view[128];
	char viewid[128];
	char lock[128];
	char interrupt[128];
	char escape_chars[128];
	char args_name[128];

	/* Check permissions */
	if (a_access) {
		char access[128];

		allowed = 0;
		clish_xmlattr_get_value_noerr(a_access, access, sizeof(access));

		if (shell->client_hooks->access_fn)
			allowed = shell->client_hooks->access_fn(shell, access);
	}
	if (!allowed)
		return;

	clish_xmlattr_get_value_noerr(a_name, name, sizeof(name));

	/* check this command doesn't already exist */
	old = clish_view_find_command(v, name, BOOL_FALSE);
	if (old) {
		/* flag the duplication then ignore further definition */
		printf("DUPLICATE COMMAND: %s\n",
		       clish_command__get_name(old));
		return;
	}

	clish_xmlattr_get_value_noerr(a_help, help, sizeof(help));

	assert(*name);
	assert(*help);

	clish_xmlattr_get_value_noerr(a_ref, ref, sizeof(ref));

	/* Reference 'ref' field */
	if (*ref) {
		char *saveptr;
		const char *delim = "@";
		char *view_name = NULL;
		char *cmdn = NULL;
		char *str = lub_string_dup(ref);

		cmdn = strtok_r(str, delim, &saveptr);
		if (!cmdn) {
			printf("EMPTY REFERENCE COMMAND: %s\n", name);
			lub_string_free(str);
			return;
		}
		alias_name = lub_string_dup(cmdn);
		view_name = strtok_r(NULL, delim, &saveptr);
		if (!view_name)
			alias_view = v;
		else
			alias_view = clish_shell_find_create_view(shell,
				view_name, NULL);
		lub_string_free(str);
	}

	clish_xmlattr_get_value_noerr(a_escape_chars, escape_chars,
		sizeof(escape_chars));
	clish_xmlattr_get_value_noerr(a_args_name, args_name,
		sizeof(args_name));

	/* create a command */
	cmd = clish_view_new_command(v, name, help);
	assert(cmd);
	clish_command__set_pview(cmd, v);
	/* define some specialist escape characters */
	if (*escape_chars)
		clish_command__set_escape_chars(cmd, escape_chars);
	if (*args_name) {
		/* define a "rest of line" argument */
		char args_help[128];
		clish_param_t *param;
		clish_ptype_t *tmp = NULL;

		clish_xmlattr_get_value_noerr(a_args_help, args_help,
			sizeof(args_help));

		assert(*args_help);
		tmp = clish_shell_find_ptype(shell, "internal_ARGS");
		assert(tmp);
		param = clish_param_new(args_name, args_help, tmp);
		clish_command__set_args(cmd, param);
	}

	clish_xmlattr_get_value_noerr(a_view, view, sizeof(view));

	/* define the view which this command changes to */
	if (*view) {
		clish_view_t *next = clish_shell_find_create_view(shell, view,
			NULL);

		/* reference the next view */
		clish_command__set_view(cmd, next);
	}

	clish_xmlattr_get_value_noerr(a_viewid, viewid, sizeof(viewid));

	/* define the view id which this command changes to */
	if (*viewid)
		clish_command__set_viewid(cmd, viewid);

	/* lock field */
	clish_xmlattr_get_value_noerr(a_lock, lock, sizeof(lock));
	if (lub_string_nocasecmp(lock, "false") == 0)
		clish_command__set_lock(cmd, BOOL_FALSE);
	else
		clish_command__set_lock(cmd, BOOL_TRUE);

	/* interrupt field */
	clish_xmlattr_get_value_noerr(a_interrupt, interrupt, sizeof(interrupt));
	if (lub_string_nocasecmp(interrupt, "true") == 0)
		clish_command__set_interrupt(cmd, BOOL_TRUE);
	else
		clish_command__set_interrupt(cmd, BOOL_FALSE);

	/* Set alias */
	if (alias_name) {
		assert(!((alias_view == v) && (!strcmp(alias_name, name))));
		clish_command__set_alias(cmd, alias_name);
		assert(alias_view);
		clish_command__set_alias_view(cmd, alias_view);
		lub_string_free(alias_name);
	}

	process_children(shell, element, cmd);
}

/* ------------------------------------------------------ */
static void
process_startup(clish_shell_t * shell, clish_xmlnode_t * element, void *parent)
{
	clish_view_t *v = (clish_view_t *) parent;
	clish_command_t *cmd = NULL;
	clish_xmlattr_t *a_view = clish_xmlnode_fetch_attr(element, "view");
	clish_xmlattr_t *a_viewid = clish_xmlnode_fetch_attr(element, "viewid");
	clish_xmlattr_t *a_default_shebang =
		clish_xmlnode_fetch_attr(element, "default_shebang");
	clish_xmlattr_t *a_timeout = clish_xmlnode_fetch_attr(element, "timeout");
	clish_xmlattr_t *a_lock = clish_xmlnode_fetch_attr(element, "lock");
	clish_xmlattr_t *a_interrupt = clish_xmlnode_fetch_attr(element, "interrupt");

	char view[128];
	char viewid[128];
	char timeout[128];
	char lock[128];
	char interrupt[128];
	char default_shebang[128];

	assert(!shell->startup);

	clish_xmlattr_get_value_noerr(a_view, view, sizeof(view));

	assert(*view);

	/* create a command with NULL help */
	cmd = clish_view_new_command(v, "startup", NULL);
	clish_command__set_lock(cmd, BOOL_FALSE);

	/* define the view which this command changes to */
	clish_view_t *next = clish_shell_find_create_view(shell, view, NULL);
	/* reference the next view */
	clish_command__set_view(cmd, next);

	/* define the view id which this command changes to */
	clish_xmlattr_get_value_noerr(a_viewid, viewid, sizeof(viewid));
	if (*viewid)
		clish_command__set_viewid(cmd, viewid);

	clish_xmlattr_get_value_noerr(a_default_shebang, default_shebang,
		sizeof(default_shebang));
	if (*default_shebang)
		clish_shell__set_default_shebang(shell, default_shebang);

	clish_xmlattr_get_value_noerr(a_timeout, timeout, sizeof(timeout));
	if (*timeout)
		clish_shell__set_timeout(shell, atoi(timeout));

	/* lock field */
	clish_xmlattr_get_value_noerr(a_lock, lock, sizeof(lock));
	if (lub_string_nocasecmp(lock, "false") == 0)
		clish_command__set_lock(cmd, BOOL_FALSE);
	else
		clish_command__set_lock(cmd, BOOL_TRUE);

	/* interrupt field */
	clish_xmlattr_get_value_noerr(a_interrupt, interrupt, sizeof(interrupt));
	if (lub_string_nocasecmp(interrupt, "true") == 0)
		clish_command__set_interrupt(cmd, BOOL_TRUE);
	else
		clish_command__set_interrupt(cmd, BOOL_FALSE);

	/* remember this command */
	shell->startup = cmd;

	process_children(shell, element, cmd);
}

/* ------------------------------------------------------ */
static void
process_param(clish_shell_t * shell, clish_xmlnode_t * element, void *parent)
{
	clish_command_t *cmd = NULL;
	clish_param_t *p_param = NULL;
	clish_xmlnode_t *pelement;
	char *pname;

	pelement = clish_xmlnode_parent(element);
	pname = clish_xmlnode_get_all_name(pelement);

	if (pname && lub_string_nocasecmp(pname, "PARAM") == 0)
		p_param = (clish_param_t *)parent;
	else
		cmd = (clish_command_t *)parent;

	if (pname)
		free(pname);

	if (cmd || p_param) {
		clish_xmlattr_t *a_name = clish_xmlnode_fetch_attr(element, "name");
		clish_xmlattr_t *a_help = clish_xmlnode_fetch_attr(element, "help");
		clish_xmlattr_t *a_ptype = clish_xmlnode_fetch_attr(element, "ptype");
		clish_xmlattr_t *a_prefix = clish_xmlnode_fetch_attr(element, "prefix");
		clish_xmlattr_t *a_defval = clish_xmlnode_fetch_attr(element, "default");
		clish_xmlattr_t *a_mode = clish_xmlnode_fetch_attr(element, "mode");
		clish_xmlattr_t *a_optional = clish_xmlnode_fetch_attr(element, "optional");
		clish_xmlattr_t *a_order = clish_xmlnode_fetch_attr(element, "order");
		clish_xmlattr_t *a_value = clish_xmlnode_fetch_attr(element, "value");
		clish_xmlattr_t *a_hidden = clish_xmlnode_fetch_attr(element, "hidden");
		clish_xmlattr_t *a_test = clish_xmlnode_fetch_attr(element, "test");
		clish_xmlattr_t *a_completion = clish_xmlnode_fetch_attr(element, "completion");
		clish_param_t *param;
		clish_ptype_t *tmp = NULL;

		char name[128];
		char help[128];
		char ptype[128];
		char prefix[128];
		char defval[128];
		char mode[128];
		char optional[128];
		char order[128];
		char value[128];
		char hidden[128];
		char test[128];
		char completion[128];

		assert((!cmd) || (cmd != shell->startup));

		clish_xmlattr_get_value_noerr(a_name, name, sizeof(name));
		clish_xmlattr_get_value_noerr(a_help, help, sizeof(help));
		clish_xmlattr_get_value_noerr(a_ptype, ptype, sizeof(ptype));

		/* create a command */
		assert(*name);
		assert(*help);
		assert(*ptype);

		if (*ptype) {
			tmp = clish_shell_find_create_ptype(shell, ptype,
				NULL, NULL,
				CLISH_PTYPE_REGEXP,
				CLISH_PTYPE_NONE);
			assert(tmp);
		}
		param = clish_param_new(name, help, tmp);

		/* If prefix is set clish will emulate old optional
		 * command syntax over newer optional command mechanism.
		 * It will create nested PARAM.
		 */
		clish_xmlattr_get_value_noerr(a_prefix, prefix, sizeof(prefix));
		clish_xmlattr_get_value_noerr(a_test, test, sizeof(test));
		if (*prefix) {
			const char *ptype_name = "__SUBCOMMAND";
			clish_param_t *opt_param = NULL;

			/* Create a ptype for prefix-named subcommand that
			 * will contain the nested optional parameter. The
			 * name of ptype is hardcoded. It's not good but
			 * it's only the service ptype.
			 */
			tmp = (clish_ptype_t *)lub_bintree_find(
				&shell->ptype_tree, ptype_name);
			if (!tmp)
				tmp = clish_shell_find_create_ptype(shell,
					ptype_name, "Option", "[^\\\\]+",
					CLISH_PTYPE_REGEXP, CLISH_PTYPE_NONE);
			assert(tmp);
			opt_param = clish_param_new(prefix, help, tmp);
			clish_param__set_mode(opt_param,
				CLISH_PARAM_SUBCOMMAND);
			clish_param__set_optional(opt_param, BOOL_TRUE);

			if (*test)
				clish_param__set_test(opt_param, test);

			/* add the parameter to the command */
			if (cmd)
				clish_command_insert_param(cmd, opt_param);
			/* add the parameter to the param */
			if (p_param)
				clish_param_insert_param(p_param, opt_param);
			/* Unset cmd and set parent param to opt_param */
			cmd = NULL;
			p_param = opt_param;
		}

		clish_xmlattr_get_value_noerr(a_defval, defval, sizeof(defval));

		if (*defval)
			clish_param__set_default(param, defval);

		clish_xmlattr_get_value_noerr(a_hidden, hidden, sizeof(hidden));
		if (lub_string_nocasecmp(hidden, "true") == 0)
			clish_param__set_hidden(param, BOOL_TRUE);
		else
			clish_param__set_hidden(param, BOOL_FALSE);

		clish_xmlattr_get_value_noerr(a_mode, mode, sizeof(mode));
		if (*mode) {
			if (lub_string_nocasecmp(mode, "switch") == 0) {
				clish_param__set_mode(param,
					CLISH_PARAM_SWITCH);
				/* Force hidden attribute */
				clish_param__set_hidden(param, BOOL_TRUE);
			} else if (lub_string_nocasecmp(mode, "subcommand") == 0)
				clish_param__set_mode(param,
					CLISH_PARAM_SUBCOMMAND);
			else
				clish_param__set_mode(param,
					CLISH_PARAM_COMMON);
		}

		clish_xmlattr_get_value_noerr(a_optional, optional,
			sizeof(optional));
		if (lub_string_nocasecmp(optional, "true") == 0)
			clish_param__set_optional(param, BOOL_TRUE);
		else
			clish_param__set_optional(param, BOOL_FALSE);

		clish_xmlattr_get_value_noerr(a_order, order, sizeof(order));
		if (lub_string_nocasecmp(order, "true") == 0)
			clish_param__set_order(param, BOOL_TRUE);
		else
			clish_param__set_order(param, BOOL_FALSE);

		clish_xmlattr_get_value_noerr(a_value, value, sizeof(value));
		if (*value) {
			clish_param__set_value(param, value);
			/* Force mode to subcommand */
			clish_param__set_mode(param,
				CLISH_PARAM_SUBCOMMAND);
		}

		if (*test && !*prefix)
			clish_param__set_test(param, test);

		clish_xmlattr_get_value_noerr(a_completion, completion,
			sizeof(completion));
		if (*completion)
			clish_param__set_completion(param, completion);

		/* add the parameter to the command */
		if (cmd)
			clish_command_insert_param(cmd, param);

		/* add the parameter to the param */
		if (p_param)
			clish_param_insert_param(p_param, param);

		process_children(shell, element, param);
	}
}

/* ------------------------------------------------------ */
static void
process_action(clish_shell_t * shell, clish_xmlnode_t * element, void *parent)
{
	clish_action_t *action = NULL;
	clish_xmlattr_t *a_builtin = clish_xmlnode_fetch_attr(element, "builtin");
	clish_xmlattr_t *a_shebang = clish_xmlnode_fetch_attr(element, "shebang");
	clish_xmlnode_t *pelement = clish_xmlnode_parent(element);
	char *pname = clish_xmlnode_get_all_name(pelement);
	char *text;
	char builtin[128];
	char shebang[128];

	if (pname && lub_string_nocasecmp(pname, "VAR") == 0)
		action = clish_var__get_action((clish_var_t *)parent);
	else
		action = clish_command__get_action((clish_command_t *)parent);
	assert(action);

	if (pname)
		free(pname);

	text = clish_xmlnode_get_all_content(element);

	if (text && *text) {
		/* store the action */
		clish_action__set_script(action, text);
	}
	if (text)
		free(text);

	clish_xmlattr_get_value_noerr(a_builtin, builtin, sizeof(builtin));
	clish_xmlattr_get_value_noerr(a_shebang, shebang, sizeof(shebang));

	if (*builtin)
		clish_action__set_builtin(action, builtin);
	if (*shebang)
		clish_action__set_shebang(action, shebang);
}

/* ------------------------------------------------------ */
static void
process_detail(clish_shell_t * shell, clish_xmlnode_t * element, void *parent)
{
	clish_command_t *cmd = (clish_command_t *) parent;

	/* read the following text element */
	char *text = clish_xmlnode_get_all_content(element);

	if (text && *text) {
		/* store the action */
		clish_command__set_detail(cmd, text);
	}

	if (text)
		free(text);
}

/* ------------------------------------------------------ */
static void
process_namespace(clish_shell_t * shell, clish_xmlnode_t * element, void *parent)
{
	clish_view_t *v = (clish_view_t *) parent;
	clish_nspace_t *nspace = NULL;

	clish_xmlattr_t *a_view = clish_xmlnode_fetch_attr(element, "ref");
	clish_xmlattr_t *a_prefix = clish_xmlnode_fetch_attr(element, "prefix");
	clish_xmlattr_t *a_prefix_help = clish_xmlnode_fetch_attr(element, "prefix_help");
	clish_xmlattr_t *a_help = clish_xmlnode_fetch_attr(element, "help");
	clish_xmlattr_t *a_completion = clish_xmlnode_fetch_attr(element, "completion");
	clish_xmlattr_t *a_context_help = clish_xmlnode_fetch_attr(element, "context_help");
	clish_xmlattr_t *a_inherit = clish_xmlnode_fetch_attr(element, "inherit");
	clish_xmlattr_t *a_access = clish_xmlnode_fetch_attr(element, "access");

	char view[128];
	char prefix[128];

	char help[128];
	char completion[128];
	char context_help[128];
	char inherit[128];

	int allowed = 1;

	if (a_access) {
		char access[128];
		allowed = 0;
		clish_xmlattr_get_value_noerr(a_access, access, sizeof(access));
		if (shell->client_hooks->access_fn)
			allowed = shell->client_hooks->access_fn(shell, access);
	}
	if (!allowed)
		return;

	clish_xmlattr_get_value_noerr(a_view, view, sizeof(view));
	clish_xmlattr_get_value_noerr(a_prefix, prefix, sizeof(prefix));

	assert(*view);
	clish_view_t *ref_view = clish_shell_find_create_view(shell,
		view, NULL);
	assert(ref_view);
	/* Don't include itself without prefix */
	if ((ref_view == v) && !*prefix)
		return;
	nspace = clish_nspace_new(ref_view);
	assert(nspace);
	clish_view_insert_nspace(v, nspace);

	if (*prefix) {
		char prefix_help[128];

		clish_nspace__set_prefix(nspace, prefix);
		clish_xmlattr_get_value_noerr(a_prefix_help, prefix_help,
			sizeof(prefix_help));
		if (*prefix_help)
			clish_nspace_create_prefix_cmd(nspace,
				"prefix",
				prefix_help);
		else
			clish_nspace_create_prefix_cmd(nspace,
				"prefix",
				"Prefix for the imported commands.");
	}

	clish_xmlattr_get_value_noerr(a_help, help, sizeof(help));

	if (lub_string_nocasecmp(help, "true") == 0)
		clish_nspace__set_help(nspace, BOOL_TRUE);
	else
		clish_nspace__set_help(nspace, BOOL_FALSE);

	clish_xmlattr_get_value_noerr(a_completion, completion,
		sizeof(completion));

	if (lub_string_nocasecmp(completion, "false") == 0)
		clish_nspace__set_completion(nspace, BOOL_FALSE);
	else
		clish_nspace__set_completion(nspace, BOOL_TRUE);

	clish_xmlattr_get_value_noerr(a_context_help, context_help,
		sizeof(context_help));

	if (lub_string_nocasecmp(context_help, "true") == 0)
		clish_nspace__set_context_help(nspace, BOOL_TRUE);
	else
		clish_nspace__set_context_help(nspace, BOOL_FALSE);

	clish_xmlattr_get_value_noerr(a_inherit, inherit, sizeof(inherit));

	if (lub_string_nocasecmp(inherit, "false") == 0)
		clish_nspace__set_inherit(nspace, BOOL_FALSE);
	else
		clish_nspace__set_inherit(nspace, BOOL_TRUE);
}

/* ------------------------------------------------------ */
static void
process_config(clish_shell_t * shell, clish_xmlnode_t * element, void *parent)
{
	clish_command_t *cmd = (clish_command_t *)parent;
	clish_config_t *config;

	if (!cmd)
		return;
	config = clish_command__get_config(cmd);

	/* read the following text element */
	clish_xmlattr_t *a_operation = clish_xmlnode_fetch_attr(element, "operation");
	clish_xmlattr_t *a_priority = clish_xmlnode_fetch_attr(element, "priority");
	clish_xmlattr_t *a_pattern = clish_xmlnode_fetch_attr(element, "pattern");
	clish_xmlattr_t *a_file = clish_xmlnode_fetch_attr(element, "file");
	clish_xmlattr_t *a_splitter = clish_xmlnode_fetch_attr(element, "splitter");
	clish_xmlattr_t *a_seq = clish_xmlnode_fetch_attr(element, "sequence");
	clish_xmlattr_t *a_unique = clish_xmlnode_fetch_attr(element, "unique");
	clish_xmlattr_t *a_depth = clish_xmlnode_fetch_attr(element, "depth");

	char operation[128];
	char priority[128];
	char pattern[128];
	char file[128];
	char splitter[128];
	char seq[128];
	char unique[128];
	char depth[128];

	clish_xmlattr_get_value_noerr(a_operation, operation, sizeof(operation));

	if (!lub_string_nocasecmp(operation, "unset"))
		clish_config__set_op(config, CLISH_CONFIG_UNSET);
	else if (!lub_string_nocasecmp(operation, "none"))
		clish_config__set_op(config, CLISH_CONFIG_NONE);
	else if (!lub_string_nocasecmp(operation, "dump"))
		clish_config__set_op(config, CLISH_CONFIG_DUMP);
	else {
		clish_config__set_op(config, CLISH_CONFIG_SET);
		/* The priority if no clearly specified */
		clish_config__set_priority(config, 0x7f00);
	}

	clish_xmlattr_get_value_noerr(a_priority, priority, sizeof(priority));

	if (*priority) {
		long val = 0;
		char *endptr;
		unsigned short pri;

		val = strtol(priority, &endptr, 0);
		if (endptr == priority)
			pri = 0;
		else if (val > 0xffff)
			pri = 0xffff;
		else if (val < 0)
			pri = 0;
		else
			pri = (unsigned short)val;
		clish_config__set_priority(config, pri);
	}

	clish_xmlattr_get_value_noerr(a_pattern, pattern, sizeof(pattern));

	if (*pattern)
		clish_config__set_pattern(config, pattern);
	else
		clish_config__set_pattern(config, "^${__cmd}");

	clish_xmlattr_get_value_noerr(a_file, file, sizeof(file));

	if (*file)
		clish_config__set_file(config, file);

	clish_xmlattr_get_value_noerr(a_splitter, splitter, sizeof(splitter));

	if (lub_string_nocasecmp(splitter, "false") == 0)
		clish_config__set_splitter(config, BOOL_FALSE);
	else
		clish_config__set_splitter(config, BOOL_TRUE);

	clish_xmlattr_get_value_noerr(a_unique, unique, sizeof(unique));

	if (lub_string_nocasecmp(unique, "false") == 0)
		clish_config__set_unique(config, BOOL_FALSE);
	else
		clish_config__set_unique(config, BOOL_TRUE);

	clish_xmlattr_get_value_noerr(a_seq, seq, sizeof(seq));

	if (*seq)
		clish_config__set_seq(config, seq);
	else
		/* The entries without sequence cannot be non-unique */
		clish_config__set_unique(config, BOOL_TRUE);

	clish_xmlattr_get_value_noerr(a_depth, depth, sizeof(depth));

	if (*depth)
		clish_config__set_depth(config, depth);

}

/* ------------------------------------------------------ */
static void process_var(clish_shell_t * shell, clish_xmlnode_t * element, void *parent)
{
	clish_var_t *var = NULL;
	clish_xmlattr_t *a_name = clish_xmlnode_fetch_attr(element, "name");
	clish_xmlattr_t *a_dynamic = clish_xmlnode_fetch_attr(element, "dynamic");
	clish_xmlattr_t *a_value = clish_xmlnode_fetch_attr(element, "value");

	char name[128];
	char value[128];
	char dynamic[128];

	clish_xmlattr_get_value_noerr(a_name, name, sizeof(name));

	assert(*name);
	/* Check if this var doesn't already exist */
	var = (clish_var_t *)lub_bintree_find(&shell->var_tree, name);
	if (var) {
		printf("DUPLICATE VAR: %s\n", name);
		assert(!var);
	}

	/* Create var instance */
	var = clish_var_new(name);
	lub_bintree_insert(&shell->var_tree, var);

	clish_xmlattr_get_value_noerr(a_dynamic, dynamic, sizeof(dynamic));

	if (lub_string_nocasecmp(dynamic, "true") == 0)
		clish_var__set_dynamic(var, BOOL_TRUE);

	clish_xmlattr_get_value_noerr(a_value, value, sizeof(value));
	if (*value)
		clish_var__set_value(var, value);

	process_children(shell, element, var);
}

/* ------------------------------------------------------ */
static void process_wdog(clish_shell_t *shell,
	clish_xmlnode_t *element, void *parent)
{
	clish_view_t *v = (clish_view_t *)parent;
	clish_command_t *cmd = NULL;

	assert(!shell->wdog);

	/* create a command with NULL help */
	cmd = clish_view_new_command(v, "watchdog", NULL);
	clish_command__set_lock(cmd, BOOL_FALSE);

	/* Remember this command */
	shell->wdog = cmd;

	process_children(shell, element, cmd);
}

/* ------------------------------------------------------ */
int clish_shell_xml_read(clish_shell_t * shell, const char *filename)
{
	int ret = -1;
	clish_xmldoc_t *doc;

	doc = clish_xmldoc_read(filename);

	if (clish_xmldoc_is_valid(doc)) {
		clish_xmlnode_t *root = clish_xmldoc_get_root(doc);
		process_node(shell, root, NULL);
		ret = 0;
	} else {
		int errcaps = clish_xmldoc_error_caps(doc);
		printf("Unable to open file '%s'", filename);
		if ((errcaps & CLISH_XMLERR_LINE) == CLISH_XMLERR_LINE)
			printf(", at line %d", clish_xmldoc_get_err_line(doc));
		if ((errcaps & CLISH_XMLERR_COL) == CLISH_XMLERR_COL)
			printf(", at column %d", clish_xmldoc_get_err_col(doc));
		if ((errcaps & CLISH_XMLERR_DESC) == CLISH_XMLERR_DESC)
			printf(", message is %s", clish_xmldoc_get_err_msg(doc));
		printf("\n");
	}

	clish_xmldoc_release(doc);

	return ret;
}

/* ------------------------------------------------------ */
