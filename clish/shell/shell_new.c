/*
 * shell_new.c
 */
#include "private.h"

#include <assert.h>
#include <stdlib.h>
/*-------------------------------------------------------- */
static void
clish_shell_init(clish_shell_t * this,
		 const clish_shell_hooks_t * hooks,
		 void *cookie, FILE * istream, FILE * ostream)
{
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
	clish_shell_iterator_init(&this->iter, CLISH_NSPACE_NONE);
	this->tinyrl = clish_shell_tinyrl_new(istream, ostream, 0);
	this->current_file = NULL;
	this->cfg_pwdv = NULL;
	this->cfg_pwdc = 0;
	this->client = konf_client_new(KONFD_SOCKET_PATH);
	this->completion_pargv = NULL;

	/* Create internal ptypes */
/*	const char *ptype_name = "__SUBCOMMAND";
			clish_param_t *opt_param = NULL;
	tmp = clish_shell_find_create_ptype(this,
		ptype_name, "Depth", "[^\\]+",
		CLISH_PTYPE_REGEXP, CLISH_PTYPE_NONE);
			assert(tmp);
			opt_param = clish_param_new(prefix, help, tmp);
			clish_param__set_mode(opt_param,
					      CLISH_PARAM_SUBCOMMAND);
			clish_param__set_optional(opt_param, BOOL_TRUE);
*/
}

/*-------------------------------------------------------- */
clish_shell_t *clish_shell_new(const clish_shell_hooks_t * hooks,
		void *cookie, FILE * istream, FILE * ostream)
{
	clish_shell_t *this = malloc(sizeof(clish_shell_t));

	if (this) {
		clish_shell_init(this, hooks, cookie, 
			istream, ostream);

		if (hooks->init_fn) {
			/* now call the client initialisation */
			if (BOOL_TRUE != hooks->init_fn(this)) {
				this->state = SHELL_STATE_CLOSING;
			}
		}
	}
	return this;
}

/*-------------------------------------------------------- */
