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
		 void *cookie, FILE * istream)
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
	this->tinyrl = clish_shell_tinyrl_new(istream, stdout, 0);
	this->current_file = NULL;
	this->cfg_pwdv = NULL;
	this->cfg_pwdc = 0;
	this->client = conf_client_new(CONFD_SOCKET_PATH);
	this->completion_pargv = NULL;
}

/*-------------------------------------------------------- */
clish_shell_t *clish_shell_new(const clish_shell_hooks_t * hooks,
			       void *cookie, FILE * istream)
{
	clish_shell_t *this = malloc(sizeof(clish_shell_t));

	if (this) {
		clish_shell_init(this, hooks, cookie, istream);

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
