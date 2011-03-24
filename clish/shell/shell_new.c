/*
 * shell_new.c
 */
#include "private.h"

#include <assert.h>
#include <stdlib.h>

#include "lub/string.h"

/*-------------------------------------------------------- */
static void
clish_shell_init(clish_shell_t * this,
	const clish_shell_hooks_t * hooks,
	void *cookie, FILE * istream,
	FILE * ostream,
	bool_t stop_on_error)
{
	clish_ptype_t *tmp_ptype = NULL;

	/* initialise the tree of views */
	lub_bintree_init(&this->view_tree,
		clish_view_bt_offset(),
		clish_view_bt_compare, clish_view_bt_getkey);

	/* initialise the tree of ptypes */
	lub_bintree_init(&this->ptype_tree,
		clish_ptype_bt_offset(),
		clish_ptype_bt_compare, clish_ptype_bt_getkey);

	assert((NULL != hooks) && (NULL != hooks->script_fn));

	/* set up defaults */
	this->client_hooks = hooks;
	this->client_cookie = cookie;
	this->view = NULL;
	this->viewid = NULL;
	this->global = NULL;
	this->startup = NULL;
	this->state = SHELL_STATE_INITIALISING;
	this->overview = NULL;
	this->tinyrl = clish_shell_tinyrl_new(istream, ostream, 0);
	this->current_file = NULL;
	this->cfg_pwdv = NULL;
	this->cfg_pwdc = 0;
	this->client = NULL;
	this->lockfile = lub_string_dup(CLISH_LOCK_PATH);
	this->default_shebang = lub_string_dup("/bin/sh");
	this->fifo_name = NULL;
	this->interactive = BOOL_TRUE; /* The interactive shell by default. */

	/* Create internal ptypes and params */
	/* Current depth */
	tmp_ptype = clish_shell_find_create_ptype(this,
		"__DEPTH", "Depth", "[0-9]+",
		CLISH_PTYPE_REGEXP, CLISH_PTYPE_NONE);
	assert(tmp_ptype);
	this->param_depth = clish_param_new("__cur_depth",
		"Current depth", tmp_ptype);
	clish_param__set_hidden(this->param_depth, BOOL_TRUE);
	/* Current pwd */
	tmp_ptype = clish_shell_find_create_ptype(this,
		"__PWD", "Path", ".+",
		CLISH_PTYPE_REGEXP, CLISH_PTYPE_NONE);
	assert(tmp_ptype);
	this->param_pwd = clish_param_new("__cur_pwd",
		"Current path", tmp_ptype);
	clish_param__set_hidden(this->param_pwd, BOOL_TRUE);
	/* Interactive */
	tmp_ptype = clish_shell_find_create_ptype(this,
		"__INTERACTIVE", "Interactive flag", "[01]",
		CLISH_PTYPE_REGEXP, CLISH_PTYPE_NONE);
	assert(tmp_ptype);
	this->param_interactive = clish_param_new("__interactive",
		"Interactive flag", tmp_ptype);
	clish_param__set_hidden(this->param_interactive, BOOL_TRUE);

	/* Initialize context */
	clish_shell_iterator_init(&this->context.iter, CLISH_NSPACE_NONE);

	/* Push non-NULL istream */
	if (istream)
		clish_shell_push_fd(this, istream, stop_on_error);
}

/*-------------------------------------------------------- */
clish_shell_t *clish_shell_new(const clish_shell_hooks_t * hooks,
	void *cookie,
	FILE * istream,
	FILE * ostream,
	bool_t stop_on_error)
{
	clish_shell_t *this = malloc(sizeof(clish_shell_t));

	if (this) {
		clish_shell_init(this, hooks, cookie,
			istream, ostream, stop_on_error);
		if (hooks->init_fn) {
			/* now call the client initialisation */
			if (BOOL_TRUE != hooks->init_fn(this))
				this->state = SHELL_STATE_CLOSING;
		}
	}

	return this;
}

/*-------------------------------------------------------- */
