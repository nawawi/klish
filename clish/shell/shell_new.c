/*
 * shell_new.c
 */
#include "private.h"

#include <assert.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <syslog.h>
#include <limits.h>

#include "lub/string.h"
#include "lub/db.h"
#include "lub/list.h"
#include "clish/plugin.h"

/*-------------------------------------------------------- */
static void clish_shell_init(clish_shell_t * this,
	FILE * istream, FILE * ostream, bool_t stop_on_error)
{
	clish_ptype_t *tmp_ptype = NULL;
	int i;
	char template[PATH_MAX];

	/* Initialise VIEW list */
	this->view_tree = lub_list_new(clish_view_compare, clish_view_delete);

	/* Init PTYPE list */
	this->ptype_tree = lub_list_new(clish_ptype_compare, clish_ptype_free);

	/* initialise the tree of vars */
	lub_bintree_init(&this->var_tree,
		clish_var_bt_offset(),
		clish_var_bt_compare, clish_var_bt_getkey);

	/* Initialize plugin list */
	this->plugins = lub_list_new(NULL, clish_plugin_free);

	/* Initialise the list of unresolved (yet) symbols */
	this->syms = lub_list_new(clish_sym_compare, clish_sym_free);

	/* Create userdata storage */
	this->udata = lub_list_new(clish_udata_compare, clish_udata_delete);

	/* Hooks */
	for (i = 0; i < CLISH_SYM_TYPE_MAX; i++) {
		this->hooks[i] = clish_sym_new(NULL, NULL, i);
		this->hooks_use[i] = BOOL_FALSE;
	}

	/* Set up defaults */
	this->global = NULL;
	this->startup = NULL;
	this->idle_timeout = 0; /* No idle timeout by default */
	this->wdog = NULL;
	this->wdog_timeout = 0; /* No watchdog timeout by default */
	this->wdog_active = BOOL_FALSE;
	this->state = SHELL_STATE_INITIALISING;
	this->overview = NULL;
	this->tinyrl = clish_shell_tinyrl_new(istream, ostream, 0);
	this->current_file = NULL;
	this->pwdv = NULL;
	this->pwdc = 0;
	this->depth = -1; /* Current depth is undefined */
	this->client = NULL;
	this->lockfile = lub_string_dup(CLISH_LOCK_PATH);
	this->default_shebang = lub_string_dup("/bin/sh");
	this->interactive = BOOL_TRUE; /* The interactive shell by default. */
	this->log = BOOL_FALSE; /* Disable logging by default */
	this->log_facility = LOG_LOCAL0; /* LOCAL0 for compatibility */
	this->dryrun = BOOL_FALSE; /* Disable dry-run by default */
	this->user = lub_db_getpwuid(getuid()); /* Get user information */
	this->default_plugin = BOOL_TRUE; /* Load default plugin by default */
	this->canon_out = BOOL_FALSE; /* A canonical output is needed in special cases only */

	/* Create template (string) for FIFO name generation */
	snprintf(template, sizeof(template),
		"%s/klish.fifo.%u.XXXXXX", "/tmp", getpid());
	template[sizeof(template) - 1] = '\0';
	this->fifo_temp = lub_string_dup(template);

	/* Create internal ptypes and params */
	/* Args */
	tmp_ptype = clish_shell_find_create_ptype(this,
		"__ptype_ARGS",
		"Arguments", "[^\\\\]+",
		CLISH_PTYPE_METHOD_REGEXP,
		CLISH_PTYPE_PRE_NONE);
	assert(tmp_ptype);

	/* Push non-NULL istream */
	if (istream)
		clish_shell_push_fd(this, istream, stop_on_error);
}

/*--------------------------------------------------------- */
static void clish_shell_fini(clish_shell_t *this)
{
	clish_var_t *var;
	unsigned int i;

	/* Free all loaded plugins */
	lub_list_free_all(this->plugins);

	/* Delete each VIEW  */
	lub_list_free_all(this->view_tree);

	/* Delete each PTYPE  */
	lub_list_free_all(this->ptype_tree);

	/* delete each VAR held  */
	while ((var = lub_bintree_findfirst(&this->var_tree))) {
		lub_bintree_remove(&this->var_tree, var);
		clish_var_delete(var);
	}

	/* Free empty hooks */
	for (i = 0; i < CLISH_SYM_TYPE_MAX; i++) {
		if (clish_sym__get_name(this->hooks[i]))
			continue;
		clish_sym_free(this->hooks[i]);
	}

	/* Free symbol list */
	lub_list_free_all(this->syms);

	/* Free user data storage */
	lub_list_free_all(this->udata);

	/* free the textual details */
	lub_string_free(this->overview);

	/* Remove the startup command */
	if (this->startup)
		clish_command_delete(this->startup);
	/* Remove the watchdog command */
	if (this->wdog)
		clish_command_delete(this->wdog);
	/* clean up the file stack */
	while (!clish_shell_pop_file(this));
	/* delete the tinyrl object */
	clish_shell_tinyrl_delete(this->tinyrl);

	/* finalize each of the pwd strings */
	for (i = 0; i < this->pwdc; i++) {
		clish_shell__fini_pwd(this->pwdv[i]);
		free(this->pwdv[i]);
	}
	/* free the pwd vector */
	free(this->pwdv);
	konf_client_free(this->client);

	lub_string_free(this->lockfile);
	lub_string_free(this->default_shebang);
	free(this->user);
	if (this->fifo_temp)
		lub_string_free(this->fifo_temp);
}

/*-------------------------------------------------------- */
clish_shell_t *clish_shell_new(
	FILE * istream,
	FILE * ostream,
	bool_t stop_on_error)
{
	clish_shell_t *this = malloc(sizeof(clish_shell_t));

	if (this)
		clish_shell_init(this, istream, ostream, stop_on_error);

	return this;
}

/*--------------------------------------------------------- */
void clish_shell_delete(clish_shell_t *this)
{
	clish_shell_fini(this);
	free(this);
}

CLISH_GET(shell, struct passwd *, user);
