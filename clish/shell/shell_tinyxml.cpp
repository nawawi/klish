////////////////////////////////////////
// shell_tinyxml_read.cpp
// 
// This file implements the means to read an XML encoded file and populate the 
// CLI tree based on the contents.
////////////////////////////////////////
extern "C" {
#include "private.h"
#include "lub/string.h"
#include "lub/ctype.h"
}
/*lint +libh(tinyxml/tinyxml.h) Add this to the library file list */
#include <stdlib.h>
#include "tinyxml/tinyxml.h"
#include <string.h>
#include <assert.h>

typedef void (PROCESS_FN) (clish_shell_t * instance,
	TiXmlElement * element, void *parent);

// Define a control block for handling the decode of an XML file
typedef struct clish_xml_cb_s clish_xml_cb_t;
struct clish_xml_cb_s {
	const char *element;
	PROCESS_FN *handler;
};

// forward declare the handler functions
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

///////////////////////////////////////
// This function reads an element from the XML stream and processes it.
///////////////////////////////////////
static void process_node(clish_shell_t * shell, TiXmlNode * node, void *parent)
{
	switch (node->Type()) {
	case TiXmlNode::DOCUMENT:
		break;
	case TiXmlNode::ELEMENT:
		clish_xml_cb_t * cb;
		for (cb = &xml_elements[0]; cb->element; cb++) {
			if (0 == strcmp(node->Value(), cb->element)) {
				TiXmlElement *element = (TiXmlElement *) node;
#ifdef DEBUG
				fprintf(stderr, "NODE:");
				element->Print(stderr, 0);
				fprintf(stderr, "\n***\n");
#endif
				// process the elements at this level
				cb->handler(shell, element, parent);
				break;
			}
		}
		break;
	case TiXmlNode::COMMENT:
	case TiXmlNode::TEXT:
	case TiXmlNode::DECLARATION:
	case TiXmlNode::TYPECOUNT:
	case TiXmlNode::UNKNOWN:
	default:
		break;
	}
}

///////////////////////////////////////
static void process_children(clish_shell_t * shell,
	TiXmlElement * element, void *parent = NULL)
{
	for (TiXmlNode * node = element->FirstChild();
		node; node = element->IterateChildren(node)) {
		// Now deal with all the contained elements
		process_node(shell, node, parent);
	}
}

///////////////////////////////////////
static void
process_clish_module(clish_shell_t * shell, TiXmlElement * element, void *)
{
	// create the global view
	if (!shell->global)
		shell->global = clish_shell_find_create_view(shell,
			"global", "");
	process_children(shell, element, shell->global);
}

///////////////////////////////////////
static void process_view(clish_shell_t * shell, TiXmlElement * element, void *)
{
	clish_view_t *view;
	const char *name = element->Attribute("name");
	const char *prompt = element->Attribute("prompt");
	const char *depth = element->Attribute("depth");
	const char *restore = element->Attribute("restore");
	const char *access = element->Attribute("access");
	bool allowed = true;

	/* Check permissions */
	if (access) {
		allowed = false;
		if (shell->client_hooks->access_fn)
			allowed = shell->client_hooks->access_fn(shell, access)
				? true : false;
	}
	if (!allowed)
		return;

	// re-use a view if it already exists
	view = clish_shell_find_create_view(shell, name, prompt);

	if (depth && (*depth != '\0') && (lub_ctype_isdigit(*depth))) {
		unsigned res = atoi(depth);
		clish_view__set_depth(view, res);
	}

	if (restore) {
		if (!lub_string_nocasecmp(restore, "depth"))
			clish_view__set_restore(view, CLISH_RESTORE_DEPTH);
		else if (!lub_string_nocasecmp(restore, "view"))
			clish_view__set_restore(view, CLISH_RESTORE_VIEW);
		else
			clish_view__set_restore(view, CLISH_RESTORE_NONE);
	}

	process_children(shell, element, view);
}

///////////////////////////////////////
static void process_ptype(clish_shell_t * shell, TiXmlElement * element, void *)
{
	clish_ptype_method_e method;
	clish_ptype_preprocess_e preprocess;
	clish_ptype_t *ptype;

	const char *name = element->Attribute("name");
	const char *help = element->Attribute("help");
	const char *pattern = element->Attribute("pattern");
	const char *method_name = element->Attribute("method");
	const char *preprocess_name = element->Attribute("preprocess");

	assert(name);
	assert(pattern);
	method = clish_ptype_method_resolve(method_name);
	preprocess = clish_ptype_preprocess_resolve(preprocess_name);
	ptype = clish_shell_find_create_ptype(shell,
		name, help, pattern, method, preprocess);
	assert(ptype);
}

///////////////////////////////////////
static void
process_overview(clish_shell_t * shell, TiXmlElement * element, void *)
{
	// read the following text element
	TiXmlNode *text = element->FirstChild();

	if (text) {
		assert(TiXmlNode::TEXT == text->Type());
		// set the overview text for this view
		assert(NULL == shell->overview);
		// store the overview
		shell->overview = lub_string_dup(text->Value());
	}
}

////////////////////////////////////////
static void
process_command(clish_shell_t * shell, TiXmlElement * element, void *parent)
{
	clish_view_t *v = (clish_view_t *) parent;
	clish_command_t *cmd = NULL;
	bool allowed = true;
	clish_command_t *old;
	char *alias_name = NULL;
	clish_view_t *alias_view = NULL;

	const char *access = element->Attribute("access");
	const char *name = element->Attribute("name");
	const char *help = element->Attribute("help");
	const char *view = element->Attribute("view");
	const char *viewid = element->Attribute("viewid");
	const char *escape_chars = element->Attribute("escape_chars");
	const char *args_name = element->Attribute("args");
	const char *args_help = element->Attribute("args_help");
	const char *lock = element->Attribute("lock");
	const char *interrupt = element->Attribute("interrupt");
	const char *ref = element->Attribute("ref");

	/* Check permissions */
	if (access) {
		allowed = false;
		if (shell->client_hooks->access_fn)
			allowed = shell->client_hooks->access_fn(shell, access)
				? true : false;
	}
	if (!allowed)
		return;

	old = clish_view_find_command(v, name, BOOL_FALSE);
	// check this command doesn't already exist
	if (old) {
		// flag the duplication then ignore further definition
		printf("DUPLICATE COMMAND: %s\n",
		       clish_command__get_name(old));
		return;
	}

	assert(name);
	assert(help);

	/* Reference 'ref' field */
	if (ref) {
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

	/* create a command */
	cmd = clish_view_new_command(v, name, help);
	assert(cmd);
	clish_command__set_pview(cmd, v);
	/* define some specialist escape characters */
	if (escape_chars)
		clish_command__set_escape_chars(cmd, escape_chars);
	if (args_name) {
		/* define a "rest of line" argument */
		clish_param_t *param;
		clish_ptype_t *tmp = NULL;

		assert(args_help);
		tmp = clish_shell_find_ptype(shell, "internal_ARGS");
		assert(tmp);
		param = clish_param_new(args_name, args_help, tmp);
		clish_command__set_args(cmd, param);
	}
	// define the view which this command changes to
	if (view) {
		clish_view_t *next = clish_shell_find_create_view(shell, view,
			NULL);

		// reference the next view
		clish_command__set_view(cmd, next);
	}
	// define the view id which this command changes to
	if (viewid)
		clish_command__set_viewid(cmd, viewid);
	/* lock field */
	if (lock && (lub_string_nocasecmp(lock, "false") == 0))
		clish_command__set_lock(cmd, BOOL_FALSE);
	else
		clish_command__set_lock(cmd, BOOL_TRUE);
	/* interrupt field */
	if (interrupt && (lub_string_nocasecmp(interrupt, "true") == 0))
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

///////////////////////////////////////
static void
process_startup(clish_shell_t * shell, TiXmlElement * element, void *parent)
{
	clish_view_t *v = (clish_view_t *) parent;
	clish_command_t *cmd = NULL;
	const char *view = element->Attribute("view");
	const char *viewid = element->Attribute("viewid");
	const char *default_shebang = element->Attribute("default_shebang");
	const char *timeout = element->Attribute("timeout");
	const char *lock = element->Attribute("lock");
	const char *interrupt = element->Attribute("interrupt");

	assert(!shell->startup);
	assert(view);

	/* create a command with NULL help */
	cmd = clish_view_new_command(v, "startup", NULL);
	clish_command__set_lock(cmd, BOOL_FALSE);

	// define the view which this command changes to
	clish_view_t *next = clish_shell_find_create_view(shell, view, NULL);
	// reference the next view
	clish_command__set_view(cmd, next);

	// define the view id which this command changes to
	if (viewid)
		clish_command__set_viewid(cmd, viewid);

	if (default_shebang)
		clish_shell__set_default_shebang(shell, default_shebang);

	if (timeout)
		clish_shell__set_timeout(shell, atoi(timeout));

	/* lock field */
	if (lock && (lub_string_nocasecmp(lock, "false") == 0))
		clish_command__set_lock(cmd, BOOL_FALSE);
	else
		clish_command__set_lock(cmd, BOOL_TRUE);

	/* interrupt field */
	if (interrupt && (lub_string_nocasecmp(interrupt, "true") == 0))
		clish_command__set_interrupt(cmd, BOOL_TRUE);
	else
		clish_command__set_interrupt(cmd, BOOL_FALSE);

	// remember this command
	shell->startup = cmd;

	process_children(shell, element, cmd);
}

///////////////////////////////////////
static void
process_param(clish_shell_t * shell, TiXmlElement * element, void *parent)
{
	clish_command_t *cmd = NULL;
	clish_param_t *p_param = NULL;
	if (0 == lub_string_nocasecmp(element->Parent()->Value(), "PARAM"))
		p_param = (clish_param_t *)parent;
	else
		cmd = (clish_command_t *)parent;

	if (cmd || p_param) {
		assert((!cmd) || (cmd != shell->startup));
		const char *name = element->Attribute("name");
		const char *help = element->Attribute("help");
		const char *ptype = element->Attribute("ptype");
		const char *prefix = element->Attribute("prefix");
		const char *defval = element->Attribute("default");
		const char *mode = element->Attribute("mode");
		const char *optional = element->Attribute("optional");
		const char *order = element->Attribute("order");
		const char *value = element->Attribute("value");
		const char *hidden = element->Attribute("hidden");
		const char *test = element->Attribute("test");
		const char *completion = element->Attribute("completion");
		clish_param_t *param;
		clish_ptype_t *tmp = NULL;

		// create a command
		assert(name);
		assert(help);
		assert(ptype);
		if (ptype && *ptype) {
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
		if (prefix) {
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

			if (test)
				clish_param__set_test(opt_param, test);

			if (cmd)
				// add the parameter to the command
				clish_command_insert_param(cmd, opt_param);
			if (p_param)
				// add the parameter to the param
				clish_param_insert_param(p_param, opt_param);
			/* Unset cmd and set parent param to opt_param */
			cmd = NULL;
			p_param = opt_param;
		}

		if (defval)
			clish_param__set_default(param, defval);

		if (hidden && (lub_string_nocasecmp(hidden, "true") == 0))
			clish_param__set_hidden(param, BOOL_TRUE);
		else
			clish_param__set_hidden(param, BOOL_FALSE);

		if (mode) {
			if (!lub_string_nocasecmp(mode, "switch")) {
				clish_param__set_mode(param,
					CLISH_PARAM_SWITCH);
				/* Force hidden attribute */
				clish_param__set_hidden(param, BOOL_TRUE);
			} else if (!lub_string_nocasecmp(mode, "subcommand"))
				clish_param__set_mode(param,
					CLISH_PARAM_SUBCOMMAND);
			else
				clish_param__set_mode(param,
					CLISH_PARAM_COMMON);
		}

		if (optional && (lub_string_nocasecmp(optional, "true") == 0))
			clish_param__set_optional(param, BOOL_TRUE);
		else
			clish_param__set_optional(param, BOOL_FALSE);

		if (order && (lub_string_nocasecmp(order, "true") == 0))
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

		if (cmd)
			// add the parameter to the command
			clish_command_insert_param(cmd, param);
		if (p_param)
			// add the parameter to the param
			clish_param_insert_param(p_param, param);

		process_children(shell, element, param);
	}
}

////////////////////////////////////////
static void
process_action(clish_shell_t * shell, TiXmlElement * element, void *parent)
{
	clish_action_t *action = NULL;
	TiXmlNode *text = element->FirstChild();
	const char *builtin = element->Attribute("builtin");
	const char *shebang = element->Attribute("shebang");

	if (!lub_string_nocasecmp(element->Parent()->Value(), "VAR"))
		action = clish_var__get_action((clish_var_t *)parent);
	else
		action = clish_command__get_action((clish_command_t *)parent);
	assert(action);

	if (text) {
		assert(TiXmlNode::TEXT == text->Type());
		// store the action
		clish_action__set_script(action, text->Value());
	}
	if (builtin)
		clish_action__set_builtin(action, builtin);
	if (shebang)
		clish_action__set_shebang(action, shebang);
}

////////////////////////////////////////
static void
process_detail(clish_shell_t * shell, TiXmlElement * element, void *parent)
{
	clish_command_t *cmd = (clish_command_t *) parent;

	// read the following text element
	TiXmlNode *text = element->FirstChild();

	if (cmd && text) {
		assert(TiXmlNode::TEXT == text->Type());
		// store the action
		clish_command__set_detail(cmd, text->Value());
	}
}

///////////////////////////////////////
static void
process_namespace(clish_shell_t * shell, TiXmlElement * element, void *parent)
{
	clish_view_t *v = (clish_view_t *) parent;
	clish_nspace_t *nspace = NULL;

	const char *view = element->Attribute("ref");
	const char *prefix = element->Attribute("prefix");
	const char *prefix_help = element->Attribute("prefix_help");
	const char *help = element->Attribute("help");
	const char *completion = element->Attribute("completion");
	const char *context_help = element->Attribute("context_help");
	const char *inherit = element->Attribute("inherit");
	const char *access = element->Attribute("access");
	bool allowed = true;

	if (access) {
		allowed = false;
		if (shell->client_hooks->access_fn)
			allowed = shell->client_hooks->access_fn(shell, access)
				? true : false;
	}
	if (!allowed)
		return;

	assert(view);
	clish_view_t *ref_view = clish_shell_find_create_view(shell,
		view, NULL);
	assert(ref_view);
	/* Don't include itself without prefix */
	if ((ref_view == v) && !prefix)
		return;
	nspace = clish_nspace_new(ref_view);
	assert(nspace);
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
				"Prefix for the imported commands.");
	}

	if (help && (lub_string_nocasecmp(help, "true") == 0))
		clish_nspace__set_help(nspace, BOOL_TRUE);
	else
		clish_nspace__set_help(nspace, BOOL_FALSE);

	if (completion && (lub_string_nocasecmp(completion, "false") == 0))
		clish_nspace__set_completion(nspace, BOOL_FALSE);
	else
		clish_nspace__set_completion(nspace, BOOL_TRUE);

	if (context_help && (lub_string_nocasecmp(context_help, "true") == 0))
		clish_nspace__set_context_help(nspace, BOOL_TRUE);
	else
		clish_nspace__set_context_help(nspace, BOOL_FALSE);

	if (inherit && (lub_string_nocasecmp(inherit, "false") == 0))
		clish_nspace__set_inherit(nspace, BOOL_FALSE);
	else
		clish_nspace__set_inherit(nspace, BOOL_TRUE);
}

////////////////////////////////////////
static void
process_config(clish_shell_t * shell, TiXmlElement * element, void *parent)
{
	clish_command_t *cmd = (clish_command_t *)parent;
	clish_config_t *config;

	if (!cmd)
		return;
	config = clish_command__get_config(cmd);

	// read the following text element
	const char *operation = element->Attribute("operation");
	const char *priority = element->Attribute("priority");
	const char *pattern = element->Attribute("pattern");
	const char *file = element->Attribute("file");
	const char *splitter = element->Attribute("splitter");
	const char *seq = element->Attribute("sequence");
	const char *unique = element->Attribute("unique");
	const char *depth = element->Attribute("depth");

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

	if (priority && (*priority != '\0')) {
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

	if (pattern)
		clish_config__set_pattern(config, pattern);
	else
		clish_config__set_pattern(config, "^${__cmd}");

	if (file)
		clish_config__set_file(config, file);

	if (splitter && (lub_string_nocasecmp(splitter, "false") == 0))
		clish_config__set_splitter(config, BOOL_FALSE);
	else
		clish_config__set_splitter(config, BOOL_TRUE);

	if (unique && (lub_string_nocasecmp(unique, "false") == 0))
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

}

///////////////////////////////////////
static void process_var(clish_shell_t * shell, TiXmlElement * element, void *)
{
	clish_var_t *var = NULL;
	const char *name = element->Attribute("name");
	const char *dynamic = element->Attribute("dynamic");
	const char *value = element->Attribute("value");

	assert(name);
	/* Check if this var doesn't already exist */
	var = (clish_var_t *)lub_bintree_find(&shell->var_tree, name);
	if (var) {
		printf("DUPLICATE VAR: %s\n", name);
		assert(!var);
	}

	/* Create var instance */
	var = clish_var_new(name);
	lub_bintree_insert(&shell->var_tree, var);

	if (dynamic && (lub_string_nocasecmp(dynamic, "true") == 0))
		clish_var__set_dynamic(var, BOOL_TRUE);

	if (value)
		clish_var__set_value(var, value);

	process_children(shell, element, var);
}

///////////////////////////////////////
static void process_wdog(clish_shell_t *shell,
	TiXmlElement *element, void *parent)
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

///////////////////////////////////////
int clish_shell_xml_read(clish_shell_t * shell, const char *filename)
{
	int ret = -1;
	TiXmlDocument doc;

	// keep the white space 
	TiXmlBase::SetCondenseWhiteSpace(false);

	if (doc.LoadFile(filename)) {
		TiXmlNode *child = 0;
		while ((child = doc.IterateChildren(child))) {
			process_node(shell, child, NULL);
		}
		ret = 0;
	} else {
		printf("Unable to open %s (%s at line %d, col %d)\n",
		        filename, doc.ErrorDesc(),
		        doc.ErrorRow(), doc.ErrorCol());
	}
	return ret;
}

///////////////////////////////////////
