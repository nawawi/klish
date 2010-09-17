/*
 * shell_pwd.c
 */
#include <stdlib.h>
#include <assert.h>

#include "lub/string.h"
#include "private.h"

/*--------------------------------------------------------- */
void
clish_shell__set_pwd(clish_shell_t * this, unsigned index,
	const char * line, clish_view_t * view, char * viewid)
{
	clish_shell_pwd_t **tmp;
	size_t new_size = 0;
	unsigned i;

	/* Create new element */
	if (index >= this->cfg_pwdc) {
		new_size = (index + 1) * sizeof(clish_shell_pwd_t *);
		/* resize the pwd vector */
		tmp = realloc(this->cfg_pwdv, new_size);
		assert(tmp);
		this->cfg_pwdv = tmp;
		/* Initialize new elements */
		for (i = this->cfg_pwdc; i <= index; i++) {
			clish_shell_pwd_t *pwd;

			pwd = malloc(sizeof(*pwd));
			assert(pwd);
			pwd->line = NULL;
			pwd->view = NULL;
			pwd->viewid = NULL;
			this->cfg_pwdv[i] = pwd;
		}
		this->cfg_pwdc = index + 1;
	}

	lub_string_free(this->cfg_pwdv[index]->line);
	this->cfg_pwdv[index]->line = line ? lub_string_dup(line) : NULL;
	this->cfg_pwdv[index]->view = view;
	lub_string_free(this->cfg_pwdv[index]->viewid);
	this->cfg_pwdv[index]->viewid = viewid ? lub_string_dup(viewid) : NULL;
}

char *clish_shell__get_pwd_line(const clish_shell_t * this, unsigned index)
{
	if (index >= this->cfg_pwdc)
		return NULL;

	return this->cfg_pwdv[index]->line;
}

clish_view_t *clish_shell__get_pwd_view(const clish_shell_t * this, unsigned index)
{
	if (index >= this->cfg_pwdc)
		return NULL;

	return this->cfg_pwdv[index]->view;
}

char *clish_shell__get_pwd_viewid(const clish_shell_t * this, unsigned index)
{
	if (index >= this->cfg_pwdc)
		return NULL;

	return this->cfg_pwdv[index]->viewid;
}

konf_client_t *clish_shell__get_client(const clish_shell_t * this)
{
	return this->client;
}
