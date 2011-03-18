/*
 * shell_delete.c
 */

#include <stdlib.h>
#include <unistd.h>

#include "private.h"
#include "lub/string.h"

/*--------------------------------------------------------- */
static void clish_shell_fini(clish_shell_t * this)
{
	clish_view_t *view;
	clish_ptype_t *ptype;
	unsigned i;

	/* delete each view held  */
	while ((view = lub_bintree_findfirst(&this->view_tree))) {
		/* remove the view from the tree */
		lub_bintree_remove(&this->view_tree, view);
		/* release the instance */
		clish_view_delete(view);
	}

	/* delete each ptype held  */
	while ((ptype = lub_bintree_findfirst(&this->ptype_tree))) {
		/* remove the command from the tree */
		lub_bintree_remove(&this->ptype_tree, ptype);
		/* release the instance */
		clish_ptype_delete(ptype);
	}
	/* free the textual details */
	lub_string_free(this->overview);
	lub_string_free(this->viewid);

	/* remove the startup command */
	if (this->startup)
		clish_command_delete(this->startup);
	/* clean up the file stack */
	while (BOOL_TRUE == clish_shell_pop_file(this));
	/* delete the tinyrl object */
	clish_shell_tinyrl_delete(this->tinyrl);

	/* finalize each of the pwd strings */
	for (i = 0; i < this->cfg_pwdc; i++) {
		lub_string_free(this->cfg_pwdv[i]->line);
		lub_string_free(this->cfg_pwdv[i]->viewid);
		free(this->cfg_pwdv[i]);
	}
	/* free the pwd vector */
	free(this->cfg_pwdv);
	this->cfg_pwdc = 0;
	this->cfg_pwdv = NULL;
	konf_client_free(this->client);

	/* Free internal params */
	clish_param_delete(this->param_depth);
	clish_param_delete(this->param_pwd);
	clish_param_delete(this->param_interactive);

	lub_string_free(this->lockfile);
	lub_string_free(this->default_shebang);
	if (this->fifo_name) {
		unlink(this->fifo_name);
		lub_string_free(this->fifo_name);
	}

	/* Clear the context */
	if (this->context.completion_pargv) {
		clish_pargv_delete(this->context.completion_pargv);
		this->context.completion_pargv = NULL;
	}
}

/*--------------------------------------------------------- */
void clish_shell_delete(clish_shell_t * this)
{
	/* now call the client finalisation */
	if (this->client_hooks->fini_fn)
		this->client_hooks->fini_fn(this);
	clish_shell_fini(this);

	free(this);
}

/*--------------------------------------------------------- */
