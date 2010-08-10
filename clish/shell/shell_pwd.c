/*
 * shell_pwd.c
 */
#include <stdlib.h>
#include <assert.h>

#include "lub/string.h"
#include "private.h"

/*--------------------------------------------------------- */
void
clish_shell__set_pwd(clish_shell_t * this, unsigned index, const char *element)
{
	char **tmp;
	size_t new_size = 0;
	unsigned i;

	if (index >= this->cfg_pwdc) {
		/* create new element */
		new_size = (index + 1) * sizeof(char *);
		/* resize the pwd vector */
		tmp = realloc(this->cfg_pwdv, new_size);
		assert(tmp);
		this->cfg_pwdv = tmp;
		for (i = this->cfg_pwdc; i <= index; i++)
			this->cfg_pwdv[i] = NULL;
		this->cfg_pwdc = index + 1;
	}

	lub_string_free(this->cfg_pwdv[index]);
	this->cfg_pwdv[index] = lub_string_dup(element);
}

const char *clish_shell__get_pwd(const clish_shell_t * this, unsigned index)
{
	if (index >= this->cfg_pwdc)
		return NULL;

	return this->cfg_pwdv[index];
}

conf_client_t *clish_shell__get_client(const clish_shell_t * this)
{
	return this->client;
}
