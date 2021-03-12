/** @file load.c
 * @brief Common part for XML parsing.
 *
 * Different XML parsing engines can provide a functions in a form of
 * standardized API. This code uses this API and parses XML to kscheme.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>

#include <faux/faux.h>
#include <faux/str.h>
#include <faux/error.h>
#include <klish/kscheme.h>
#include <klish/kxml.h>

#define TAG "XML"

typedef bool_t (kxml_process_f)(kxml_node_t *element, void *parent);

static kxml_process_fn
	process_scheme,
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
	process_wdog,
	process_hotkey,
	process_plugin,
	process_hook;


typedef struct kxml_cb_s kxml_cb_t;
struct kxml_cb_s {
	const char *element;
	kxml_process_fn *handler;
};

static kxml_cb_t xml_elements[] = {
	{"KLISH", process_scheme},
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
	{"HOTKEY", process_hotkey},
	{"PLUGIN", process_plugin},
	{"HOOK", process_hook},
	{NULL, NULL}
};


/** @brief Default path to get XML files from.
 */
const char *default_path = "/etc/klish;~/.klish";
static const char *path_separators = ":;";


static bool_t process_node(kxml_node_t *node, void *parent, faux_error_t *error);


static bool_t kxml_load_file(kscheme_t *scheme, const char *filename, faux_error_t *error)
{
	int res = -1;
	kxml_doc_t *doc = NULL;
	kxml_node_t *root = NULL;
	bool_t r = BOOL_FALSE;

	if (!scheme)
		return BOOL_FALSE;
	if (!filename)
		return BOOL_FALSE;

	doc = kxml_doc_read(filename);
	if (!kxml_doc_is_valid(doc)) {
		int errcaps = kxml_doc_error_caps(doc);
/*		printf("Unable to open file '%s'", filename);
		if ((errcaps & kxml_ERR_LINE) == kxml_ERR_LINE)
			printf(", at line %d", kxml_doc_get_err_line(doc));
		if ((errcaps & kxml_ERR_COL) == kxml_ERR_COL)
			printf(", at column %d", kxml_doc_get_err_col(doc));
		if ((errcaps & kxml_ERR_DESC) == kxml_ERR_DESC)
			printf(", message is %s", kxml_doc_get_err_msg(doc));
		printf("\n");
*/		kxml_doc_release(doc);
		return BOOL_FALSE;
	}
	root = kxml_doc_get_root(doc);
	r = process_node(root, scheme, error);
	kxml_doc_release(doc);
	if (!r) {
		faux_error_sprintf(error, TAG": Illegal file %s", filename);
		return BOOL_FALSE;
	}

	return BOOL_TRUE;
}


kscheme_t *kxml_load_scheme(const char *xml_path, faux_error_t *error)
{
	kscheme_t *scheme = NULL;
	const char *path = xml_path;
	char *realpath = NULL;
	char *fn = NULL;
	char *saveptr = NULL;
	bool_t ret = BOOL_TRUE;

	// New kscheme instance
	scheme = kscheme_new();
	if (!scheme)
		return NULL;

	// Use the default path if not specified
	if (!path)
		path = default_path;
	realpath = faux_expand_tilde(path);

	// Loop through each directory
	for (fn = strtok_r(realpath, path_separators, &saveptr);
		fn; fn = strtok_r(NULL, path_separators, &saveptr)) {
		DIR *dir = NULL;
		struct dirent *entry = NULL;

		// Regular file
		if (faux_isfile(fn)) {
			if (!kxml_load_file(scheme, fn, error)) {
				ret = BOOL_FALSE;
				continue;
			}
		}

		// Search this directory for any XML files
		dir = opendir(fn);
		if (!dir)
			continue;
		for (entry = readdir(dir); entry; entry = readdir(dir)) {
			const char *extension = strrchr(entry->d_name, '.');
			char *filename = NULL;

			// Check the filename
			if (!extension || strcmp(".xml", extension))
				continue;
			filename = faux_str_sprintf("%s/%s", fn, entry->d_name);
			if (!kxml_load_file(scheme, filename, error)) {
				ret = BOOL_FALSE;
				continue;
			}
			faux_str_free(filename);
		}
		closedir(dir);
	}

	faux_str_free(realpath);
	if (!ret) { // Some errors while XML parsing
		kscheme_free(scheme);
		return NULL;
	}

	return scheme;
}


/** @brief Reads an element from the XML stream and processes it.
 */
static bool_t process_node(kxml_node_t *node, void *parent, faux_error_t *error)
{
	kxml_cb_t *cb = NULL;
	char *name = NULL;
	kxml_process_fn *handler = NULL;

	if (!node)
		return BOOL_FALSE;
	if (!parent)
		return BOOL_FALSE;

	if (kxml_node_type(node) != KXML_NODE_ELM)
		return BOOL_TRUE;

	name = kxml_node_name(node);
	if (!name)
		return BOOL_TRUE; // Strange case
	// Find element handler
	for (cb = &xml_elements[0]; cb->element; cb++) {
		if (faux_str_casecmp(name, cb->element)) {
			handler = cb->handler;
			break;
		}
	}
	kxml_node_name_free(name);
	if (!handler)
		return BOOL_TRUE; // Unknown element

	return handler(node, parent, error);
}


/** @brief Iterate through element's children.
 */
static bool_t process_children(kxml_node_t *element, void *parent, faux_error_t *error)
{
	kxml_node_t *node = NULL;

	while ((node = kxml_node_next_child(element, node)) != NULL) {
		bool_t res = BOOL_FALSE;
		res = process_node(node, parent, error);
		if (!res)
			return res;
	}

	return BOOL_TRUE;
}


static bool_t process_scheme(kxml_node_t *element, void *parent, faux_error_t *error)
{
	parent = parent; // Happy compiler

	return process_children(element, parent, error);
}


static bool_t process_view(kxml_node_t *element, void *parent, faux_error_t *error)
{
	iview_t iview = {};
	clish_view_t *view = NULL;
	bool_t res = BOOL_FALSE;

	iview.name = kxml_node_attr(element, "name");

	view = iview_load(iview, error);
	if (!view)
		goto err;
	if (!process_children(shell, element, view))
		goto err;

	res = BOOL_TRUE;
err:
	kxml_node_attr_free(name);

	parent = parent; /* Happy compiler */

	return res;
}


static int process_ptype(kxml_node_t *element,
	void *parent)
{
	clish_ptype_method_e method;
	clish_ptype_preprocess_e preprocess;
	int res = -1;
	clish_ptype_t *ptype;

	char *name = kxml_node_attr(element, "name");
	char *help = kxml_node_attr(element, "help");
	char *pattern = kxml_node_attr(element, "pattern");
	char *method_name = kxml_node_attr(element, "method");
	char *preprocess_name =	kxml_node_attr(element, "preprocess");
	char *completion = kxml_node_attr(element, "completion");

	/* Check syntax */
	if (!name) {
		fprintf(stderr, kxml__ERROR_ATTR("name"));
		goto error;
	}

	method = clish_ptype_method_resolve(method_name);
	if (CLISH_PTYPE_METHOD_MAX == method) {
		fprintf(stderr, kxml__ERROR_ATTR("method"));
		goto error;
	}
	if ((method != CLISH_PTYPE_METHOD_CODE) && !pattern) {
		fprintf(stderr, kxml__ERROR_ATTR("pattern"));
		goto error;
	}

	preprocess = clish_ptype_preprocess_resolve(preprocess_name);
	ptype = clish_shell_find_create_ptype(shell,
		name, help, pattern, method, preprocess);

	if (completion)
		clish_ptype__set_completion(ptype, completion);

	res = process_children(shell, element, ptype);
error:
	kxml_node_attr_free(name);
	kxml_node_attr_free(help);
	kxml_node_attr_free(pattern);
	kxml_node_attr_free(method_name);
	kxml_node_attr_free(preprocess_name);
	kxml_node_attr_free(completion);

	parent = parent; /* Happy compiler */

	return res;
}


static int process_overview(kxml_node_t *element,
	void *parent)
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
		char *new = (char*)realloc(content, content_len);
		if (!new) {
			if (content)
				free(content);
			return -1;
		}
		content = new;
		result = kxml_node_get_content(element, content,
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

	parent = parent; /* Happy compiler */

	return 0;
}


static int process_command(kxml_node_t *element,
	void *parent)
{
	clish_view_t *v = (clish_view_t *) parent;
	clish_command_t *cmd = NULL;
	clish_command_t *old;
	int res = -1;

	char *access = kxml_node_attr(element, "access");
	char *name = kxml_node_attr(element, "name");
	char *help = kxml_node_attr(element, "help");
	char *view = kxml_node_attr(element, "view");
	char *viewid = kxml_node_attr(element, "viewid");
	char *escape_chars = kxml_node_attr(element, "escape_chars");
	char *args_name = kxml_node_attr(element, "args");
	char *args_help = kxml_node_attr(element, "args_help");
	char *ref = kxml_node_attr(element, "ref");

	/* Check syntax */
	if (!name) {
		fprintf(stderr, kxml__ERROR_ATTR("name"));
		goto error;
	}
	if (!help) {
		fprintf(stderr, kxml__ERROR_ATTR("help"));
		goto error;
	}

	/* check this command doesn't already exist */
	old = clish_view_find_command(v, name, BOOL_FALSE);
	if (old) {
		fprintf(stderr, kxml__ERROR_STR"Duplicate COMMAND name=\"%s\".\n", name);
		goto error;
	}

	/* create a command */
	cmd = clish_view_new_command(v, name, help);
	clish_command__set_pview(cmd, v);

	/* Reference 'ref' field */
	if (ref) {
		char *saveptr = NULL;
		const char *delim = "@";
		char *view_name = NULL;
		char *cmdn = NULL;
		char *str = lub_string_dup(ref);

		cmdn = strtok_r(str, delim, &saveptr);
		if (!cmdn) {
			fprintf(stderr, kxml__ERROR_STR"Invalid \"ref\" attribute value.\n");
			lub_string_free(str);
			goto error;
		}
		clish_command__set_alias(cmd, cmdn); /* alias name */
		view_name = strtok_r(NULL, delim, &saveptr);
		clish_command__set_alias_view(cmd, view_name);
		lub_string_free(str);
	}

	/* define some specialist escape characters */
	if (escape_chars)
		clish_command__set_escape_chars(cmd, escape_chars);

	if (args_name) {
		/* define a "rest of line" argument */
		clish_param_t *param;

		/* Check syntax */
		if (!args_help) {
			fprintf(stderr, kxml__ERROR_ATTR("args_help"));
			goto error;
		}
		param = clish_param_new(args_name, args_help, "__ptype_ARGS");
		clish_command__set_args(cmd, param);
	}

	/* define the view which this command changes to */
	if (view) {
		/* reference the next view */
		clish_command__set_viewname(cmd, view);
	}

	/* define the view id which this command changes to */
	if (viewid)
		clish_command__set_viewid(cmd, viewid);

	if (access)
		clish_command__set_access(cmd, access);

//process_command_end:
	res = process_children(shell, element, cmd);
error:
	kxml_node_attr_free(access);
	kxml_node_attr_free(name);
	kxml_node_attr_free(help);
	kxml_node_attr_free(view);
	kxml_node_attr_free(viewid);
	kxml_node_attr_free(escape_chars);
	kxml_node_attr_free(args_name);
	kxml_node_attr_free(args_help);
	kxml_node_attr_free(ref);

	return res;
}


static int process_startup(kxml_node_t *element,
	void *parent)
{
	clish_command_t *cmd = NULL;
	int res = -1;

	char *view = kxml_node_attr(element, "view");
	char *viewid = kxml_node_attr(element, "viewid");
	char *timeout = kxml_node_attr(element, "timeout");
	char *default_plugin = kxml_node_attr(element,
		"default_plugin");
	char *default_shebang = kxml_node_attr(element,
		"default_shebang");
	char *default_expand = kxml_node_attr(element,
		"default_expand");

	/* Check syntax */
	if (!view) {
		fprintf(stderr, kxml__ERROR_ATTR("view"));
		goto error;
	}
	if (shell->startup) {
		fprintf(stderr, kxml__ERROR_STR"STARTUP tag duplication.\n");
		goto error;
	}

	/* create a command with NULL help */
	cmd = clish_command_new("startup", NULL);
	clish_command__set_internal(cmd, BOOL_TRUE);

	/* reference the next view */
	clish_command__set_viewname(cmd, view);

	/* define the view id which this command changes to */
	if (viewid)
		clish_command__set_viewid(cmd, viewid);

	if (default_shebang)
		clish_shell__set_default_shebang(shell, default_shebang);

	if (default_expand)
		clish_shell__set_default_expand(shell,
			(lub_string_nocasecmp(default_expand, "true") == 0));

	if (timeout) {
		unsigned int to = 0;
		lub_conv_atoui(timeout, &to, 0);
		clish_shell__set_idle_timeout(shell, to);
	}

	/* If we need the default plugin */
	if (default_plugin && (0 == strcmp(default_plugin, "false")))
		shell->default_plugin = BOOL_FALSE;

	/* remember this command */
	shell->startup = cmd;

	res = process_children(shell, element, cmd);
error:
	kxml_node_attr_free(view);
	kxml_node_attr_free(viewid);
	kxml_node_attr_free(default_plugin);
	kxml_node_attr_free(default_shebang);
	kxml_node_attr_free(default_expand);
	kxml_node_attr_free(timeout);

	parent = parent; /* Happy compiler */

	return res;
}


static int process_param(kxml_node_t *element,
	void *parent)
{
	clish_command_t *cmd = NULL;
	clish_param_t *p_param = NULL;
	kxml_node_t *pelement;
	clish_param_t *param;
	char *pname;
	int res = -1;

	char *name = kxml_node_attr(element, "name");
	char *help = kxml_node_attr(element, "help");
	char *ptype = kxml_node_attr(element, "ptype");
	char *prefix = kxml_node_attr(element, "prefix");
	char *defval = kxml_node_attr(element, "default");
	char *mode = kxml_node_attr(element, "mode");
	char *optional = kxml_node_attr(element, "optional");
	char *order = kxml_node_attr(element, "order");
	char *value = kxml_node_attr(element, "value");
	char *hidden = kxml_node_attr(element, "hidden");
	char *test = kxml_node_attr(element, "test");
	char *completion = kxml_node_attr(element, "completion");
	char *access = kxml_node_attr(element, "access");

	/* The PARAM can be child of COMMAND or another PARAM */
	pelement = kxml_node_parent(element);
	pname = kxml_node_get_all_name(pelement);
	if (pname && lub_string_nocasecmp(pname, "PARAM") == 0)
		p_param = (clish_param_t *)parent;
	else
		cmd = (clish_command_t *)parent;
	if (pname)
		free(pname);
	if (!cmd && !p_param)
		goto error;

	/* Check syntax */
	if (cmd && (cmd == shell->startup)) {
		fprintf(stderr, kxml__ERROR_STR"STARTUP can't contain PARAMs.\n");
		goto error;
	}
	if (!name) {
		fprintf(stderr, kxml__ERROR_ATTR("name"));
		goto error;
	}
	if (!help) {
		fprintf(stderr, kxml__ERROR_ATTR("help"));
		goto error;
	}
	if (!ptype) {
		fprintf(stderr, kxml__ERROR_ATTR("ptype"));
		goto error;
	}

	param = clish_param_new(name, help, ptype);

	/* If prefix is set clish will emulate old optional
	 * command syntax over newer optional command mechanism.
	 * It will create nested PARAM.
	 */
	if (prefix) {
		const char *ptype_name = "__ptype_SUBCOMMAND";
		clish_param_t *opt_param = NULL;
		char *str = NULL;
		clish_ptype_t *tmp;

		/* Create a ptype for prefix-named subcommand that
		 * will contain the nested optional parameter. The
		 * name of ptype is hardcoded. It's not good but
		 * it's only the service ptype.
		 */
		tmp = clish_shell_find_create_ptype(shell,
			ptype_name, "Option", "[^\\\\]+",
			CLISH_PTYPE_METHOD_REGEXP, CLISH_PTYPE_PRE_NONE);
		assert(tmp);
		lub_string_cat(&str, "_prefix_");
		lub_string_cat(&str, name);
		opt_param = clish_param_new(str, help, ptype_name);
		lub_string_free(str);
		clish_param__set_mode(opt_param,
			CLISH_PARAM_SUBCOMMAND);
		clish_param__set_value(opt_param, prefix);
		clish_param__set_optional(opt_param, BOOL_TRUE);

		if (test)
			clish_param__set_test(opt_param, test);

		/* Add the parameter to the command */
		if (cmd)
			clish_command_insert_param(cmd, opt_param);
		/* Add the parameter to the param */
		if (p_param)
			clish_param_insert_param(p_param, opt_param);
		/* Unset cmd and set parent param to opt_param */
		cmd = NULL;
		p_param = opt_param;
	}

	if (defval)
		clish_param__set_defval(param, defval);

	if (hidden && lub_string_nocasecmp(hidden, "true") == 0)
		clish_param__set_hidden(param, BOOL_TRUE);
	else
		clish_param__set_hidden(param, BOOL_FALSE);

	if (mode) {
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

	if (optional && lub_string_nocasecmp(optional, "true") == 0)
		clish_param__set_optional(param, BOOL_TRUE);
	else
		clish_param__set_optional(param, BOOL_FALSE);

	if (order && lub_string_nocasecmp(order, "true") == 0)
		clish_param__set_order(param, BOOL_TRUE);
	else
		clish_param__set_order(param, BOOL_FALSE);

	if (value) {
		clish_param__set_value(param, value);
		/* Force mode to subcommand */
		clish_param__set_mode(param,
			CLISH_PARAM_SUBCOMMAND);
	}

	if (test && !prefix)
		clish_param__set_test(param, test);

	if (completion)
		clish_param__set_completion(param, completion);

	if (access)
		clish_param__set_access(param, access);

	/* Add the parameter to the command */
	if (cmd)
		clish_command_insert_param(cmd, param);

	/* Add the parameter to the param */
	if (p_param)
		clish_param_insert_param(p_param, param);

	res = process_children(shell, element, param);

error:
	kxml_node_attr_free(name);
	kxml_node_attr_free(help);
	kxml_node_attr_free(ptype);
	kxml_node_attr_free(prefix);
	kxml_node_attr_free(defval);
	kxml_node_attr_free(mode);
	kxml_node_attr_free(optional);
	kxml_node_attr_free(order);
	kxml_node_attr_free(value);
	kxml_node_attr_free(hidden);
	kxml_node_attr_free(test);
	kxml_node_attr_free(completion);
	kxml_node_attr_free(access);

	return res;
}


static int process_action(kxml_node_t *element,
	void *parent)
{
	clish_action_t *action = NULL;

	char *builtin = kxml_node_attr(element, "builtin");
	char *shebang = kxml_node_attr(element, "shebang");
	char *lock = kxml_node_attr(element, "lock");
	char *interrupt = kxml_node_attr(element, "interrupt");
	char *interactive = kxml_node_attr(element, "interactive");
	char *expand = kxml_node_attr(element, "expand");

	kxml_node_t *pelement = kxml_node_parent(element);
	char *pname = kxml_node_get_all_name(pelement);
	char *text;
	clish_sym_t *sym = NULL;

	if (pname && lub_string_nocasecmp(pname, "VAR") == 0)
		action = clish_var__get_action((clish_var_t *)parent);
	else if (pname && lub_string_nocasecmp(pname, "PTYPE") == 0)
		action = clish_ptype__get_action((clish_ptype_t *)parent);
	else
		action = clish_command__get_action((clish_command_t *)parent);

	if (pname)
		free(pname);

	text = kxml_node_get_all_content(element);

	if (text && *text) {
		/* store the action */
		clish_action__set_script(action, text);
	}
	if (text)
		free(text);

	if (builtin)
		sym = clish_shell_add_unresolved_sym(shell, builtin,
			CLISH_SYM_TYPE_ACTION);
	else
		sym = shell->hooks[CLISH_SYM_TYPE_ACTION];

	clish_action__set_builtin(action, sym);
	if (shebang)
		clish_action__set_shebang(action, shebang);

	/* lock */
	if (lock && lub_string_nocasecmp(lock, "false") == 0)
		clish_action__set_lock(action, BOOL_FALSE);

	/* interactive */
	if (interactive && lub_string_nocasecmp(interactive, "true") == 0)
		clish_action__set_interactive(action, BOOL_TRUE);

	/* interrupt */
	if (interrupt && lub_string_nocasecmp(interrupt, "true") == 0)
		clish_action__set_interrupt(action, BOOL_TRUE);

	/* expand */
	if (expand)
		clish_action__set_expand(action, lub_tri_from_string(expand));

	kxml_node_attr_free(builtin);
	kxml_node_attr_free(shebang);
	kxml_node_attr_free(lock);
	kxml_node_attr_free(interrupt);
	kxml_node_attr_free(interactive);
	kxml_node_attr_free(expand);

	return 0;
}


static int process_detail(kxml_node_t *element,
	void *parent)
{
	clish_command_t *cmd = (clish_command_t *) parent;

	/* read the following text element */
	char *text = kxml_node_get_all_content(element);

	if (text && *text) {
		/* store the action */
		clish_command__set_detail(cmd, text);
	}

	if (text)
		free(text);

	shell = shell; /* Happy compiler */

	return 0;
}


static int process_namespace(kxml_node_t *element,
	void *parent)
{
	clish_view_t *v = (clish_view_t *)parent;
	clish_nspace_t *nspace = NULL;
	int res = -1;

	char *view = kxml_node_attr(element, "ref");
	char *prefix = kxml_node_attr(element, "prefix");
	char *prefix_help = kxml_node_attr(element, "prefix_help");
	char *help = kxml_node_attr(element, "help");
	char *completion = kxml_node_attr(element, "completion");
	char *context_help = kxml_node_attr(element, "context_help");
	char *inherit = kxml_node_attr(element, "inherit");
	char *access = kxml_node_attr(element, "access");

	/* Check syntax */
	if (!view) {
		fprintf(stderr, kxml__ERROR_ATTR("ref"));
		goto error;
	}

	clish_view_t *ref_view = clish_shell_find_view(shell, view);

	/* Don't include itself without prefix */
	if ((ref_view == v) && !prefix)
		goto process_namespace_end;

	nspace = clish_nspace_new(view);
	clish_view_insert_nspace(v, nspace);

	if (prefix) {
		clish_nspace__set_prefix(nspace, prefix);
		if (prefix_help)
			clish_nspace_create_prefix_cmd(nspace,
				"prefix",
				prefix_help);
		else
			clish_nspace_create_prefix_cmd(nspace,
				"prefix",
				"Prefix for imported commands.");
	}

	if (help && lub_string_nocasecmp(help, "true") == 0)
		clish_nspace__set_help(nspace, BOOL_TRUE);
	else
		clish_nspace__set_help(nspace, BOOL_FALSE);

	if (completion && lub_string_nocasecmp(completion, "false") == 0)
		clish_nspace__set_completion(nspace, BOOL_FALSE);
	else
		clish_nspace__set_completion(nspace, BOOL_TRUE);

	if (context_help && lub_string_nocasecmp(context_help, "true") == 0)
		clish_nspace__set_context_help(nspace, BOOL_TRUE);
	else
		clish_nspace__set_context_help(nspace, BOOL_FALSE);

	if (inherit && lub_string_nocasecmp(inherit, "false") == 0)
		clish_nspace__set_inherit(nspace, BOOL_FALSE);
	else
		clish_nspace__set_inherit(nspace, BOOL_TRUE);

	if (access)
		clish_nspace__set_access(nspace, access);

process_namespace_end:
	res = 0;
error:
	kxml_node_attr_free(view);
	kxml_node_attr_free(prefix);
	kxml_node_attr_free(prefix_help);
	kxml_node_attr_free(help);
	kxml_node_attr_free(completion);
	kxml_node_attr_free(context_help);
	kxml_node_attr_free(inherit);
	kxml_node_attr_free(access);

	return res;
}


static int process_config(kxml_node_t *element,
	void *parent)
{
	clish_command_t *cmd = (clish_command_t *)parent;
	clish_config_t *config;

	if (!cmd)
		return 0;
	config = clish_command__get_config(cmd);

	/* read the following text element */
	char *operation = kxml_node_attr(element, "operation");
	char *priority = kxml_node_attr(element, "priority");
	char *pattern = kxml_node_attr(element, "pattern");
	char *file = kxml_node_attr(element, "file");
	char *splitter = kxml_node_attr(element, "splitter");
	char *seq = kxml_node_attr(element, "sequence");
	char *unique = kxml_node_attr(element, "unique");
	char *depth = kxml_node_attr(element, "depth");

	if (operation && !lub_string_nocasecmp(operation, "unset"))
		clish_config__set_op(config, CLISH_CONFIG_UNSET);
	else if (operation && !lub_string_nocasecmp(operation, "none"))
		clish_config__set_op(config, CLISH_CONFIG_NONE);
	else if (operation && !lub_string_nocasecmp(operation, "dump"))
		clish_config__set_op(config, CLISH_CONFIG_DUMP);
	else {
		clish_config__set_op(config, CLISH_CONFIG_SET);
		/* The priority if no clearly specified */
		clish_config__set_priority(config, 0x7f00);
	}

	if (priority) {
		unsigned short pri = 0;
		lub_conv_atous(priority, &pri, 0);
		clish_config__set_priority(config, pri);
	}

	if (pattern)
		clish_config__set_pattern(config, pattern);
	else
		clish_config__set_pattern(config, "^${__cmd}");

	if (file)
		clish_config__set_file(config, file);

	if (splitter && lub_string_nocasecmp(splitter, "false") == 0)
		clish_config__set_splitter(config, BOOL_FALSE);
	else
		clish_config__set_splitter(config, BOOL_TRUE);

	if (unique && lub_string_nocasecmp(unique, "false") == 0)
		clish_config__set_unique(config, BOOL_FALSE);
	else
		clish_config__set_unique(config, BOOL_TRUE);

	if (seq)
		clish_config__set_seq(config, seq);
	else
		/* The entries without sequence cannot be non-unique */
		clish_config__set_unique(config, BOOL_TRUE);

	if (depth)
		clish_config__set_depth(config, depth);

	kxml_node_attr_free(operation);
	kxml_node_attr_free(priority);
	kxml_node_attr_free(pattern);
	kxml_node_attr_free(file);
	kxml_node_attr_free(splitter);
	kxml_node_attr_free(seq);
	kxml_node_attr_free(unique);
	kxml_node_attr_free(depth);

	shell = shell; /* Happy compiler */

	return 0;
}


static int process_var(kxml_node_t *element,
	void *parent)
{
	clish_var_t *var = NULL;
	int res = -1;

	char *name = kxml_node_attr(element, "name");
	char *dynamic = kxml_node_attr(element, "dynamic");
	char *value = kxml_node_attr(element, "value");

	/* Check syntax */
	if (!name) {
		fprintf(stderr, kxml__ERROR_ATTR("name"));
		goto error;
	}
	/* Check if this var doesn't already exist */
	var = (clish_var_t *)lub_bintree_find(&shell->var_tree, name);
	if (var) {
		fprintf(stderr, kxml__ERROR_STR"Duplicate VAR name=\"%s\".\n", name);
		goto error;
	}

	/* Create var instance */
	var = clish_var_new(name);
	lub_bintree_insert(&shell->var_tree, var);

	if (dynamic && lub_string_nocasecmp(dynamic, "true") == 0)
		clish_var__set_dynamic(var, BOOL_TRUE);

	if (value)
		clish_var__set_value(var, value);

	res = process_children(shell, element, var);
error:
	kxml_node_attr_free(name);
	kxml_node_attr_free(dynamic);
	kxml_node_attr_free(value);

	parent = parent; /* Happy compiler */

	return res;
}


static int process_wdog(clish_shell_t *shell,
	kxml_node_t *element, void *parent)
{
	clish_command_t *cmd = NULL;
	int res = -1;

	/* Check syntax */
	if (shell->wdog) {
		fprintf(stderr, kxml__ERROR_STR"WATCHDOG tag duplication.\n");
		goto error;
	}

	/* Create a command with NULL help */
	cmd = clish_command_new("watchdog", NULL);

	/* Remember this command */
	shell->wdog = cmd;

	res = process_children(shell, element, cmd);
error:
	parent = parent; /* Happy compiler */

	return res;
}


static int process_hotkey(kxml_node_t *element,
	void *parent)
{
	clish_view_t *v = (clish_view_t *)parent;
	int res = -1;

	char *key = kxml_node_attr(element, "key");
	char *cmd = kxml_node_attr(element, "cmd");

	/* Check syntax */
	if (!key) {
		fprintf(stderr, kxml__ERROR_ATTR("key"));
		goto error;
	}
	if (!cmd) {
		fprintf(stderr, kxml__ERROR_ATTR("cmd"));
		goto error;
	}

	clish_view_insert_hotkey(v, key, cmd);

	res = 0;
error:
	kxml_node_attr_free(key);
	kxml_node_attr_free(cmd);

	shell = shell; /* Happy compiler */

	return res;
}


static int process_plugin(kxml_node_t *element,
	void *parent)
{
	clish_plugin_t *plugin;
	char *file = kxml_node_attr(element, "file");
	char *name = kxml_node_attr(element, "name");
	char *alias = kxml_node_attr(element, "alias");
	char *rtld_global = kxml_node_attr(element, "rtld_global");
	int res = -1;
	char *text;

	/* Check syntax */
	if (!name) {
		fprintf(stderr, kxml__ERROR_ATTR("name"));
		goto error;
	}

	plugin = clish_shell_find_plugin(shell, name);
	if (plugin) {
		fprintf(stderr,
			kxml__ERROR_STR"PLUGIN %s duplication.\n", name);
		goto error;
	}
	plugin = clish_shell_create_plugin(shell, name);

	if (alias && *alias)
		clish_plugin__set_alias(plugin, alias);

	if (file && *file)
		clish_plugin__set_file(plugin, file);

	if (rtld_global && lub_string_nocasecmp(rtld_global, "true") == 0)
		clish_plugin__set_rtld_global(plugin, BOOL_TRUE);

	/* Get PLUGIN body content */
	text = kxml_node_get_all_content(element);
	if (text && *text)
		clish_plugin__set_conf(plugin, text);
	if (text)
		free(text);

	res = 0;
error:
	kxml_node_attr_free(file);
	kxml_node_attr_free(name);
	kxml_node_attr_free(alias);
	kxml_node_attr_free(rtld_global);

	parent = parent; /* Happy compiler */

	return res;
}


static int process_hook(kxml_node_t *element,
	void *parent)
{
	char *name = kxml_node_attr(element, "name");
	char *builtin = kxml_node_attr(element, "builtin");
	int res = -1;
	int type = CLISH_SYM_TYPE_NONE;

	/* Check syntax */
	if (!name) {
		fprintf(stderr, kxml__ERROR_ATTR("name"));
		goto error;
	}
	/* Find out HOOK type */
	if (!strcmp(name, "action"))
		type = CLISH_SYM_TYPE_ACTION;
	else if (!strcmp(name, "access"))
		type = CLISH_SYM_TYPE_ACCESS;
	else if (!strcmp(name, "config"))
		type = CLISH_SYM_TYPE_CONFIG;
	else if (!strcmp(name, "log"))
		type = CLISH_SYM_TYPE_LOG;
	if (CLISH_SYM_TYPE_NONE == type) {
		fprintf(stderr, kxml__ERROR_STR"Unknown HOOK name %s.\n", name);
		goto error;
	}

	/* Check duplicate HOOK tag */
	if (shell->hooks_use[type]) {
		fprintf(stderr,
			kxml__ERROR_STR"HOOK %s duplication.\n", name);
		goto error;
	}
	shell->hooks_use[type] = BOOL_TRUE;
	clish_sym__set_name(shell->hooks[type], builtin);

	res = 0;
error:
	kxml_node_attr_free(name);
	kxml_node_attr_free(builtin);

	parent = parent; /* Happy compiler */

	return res;
}
