#include <stdlib.h>
#include <assert.h>

#include "private.h"

bool_t
clish_shell_push_file(clish_shell_t * this, const char * fname, bool_t stop_on_error)
{
	FILE *file;
	bool_t res;

	assert(this);
	if (!fname)
		return BOOL_FALSE;
	file = fopen(fname, "r");
	if (!file)
		return BOOL_FALSE;
	res = clish_shell_push_fd(this, file, stop_on_error);
	if (!res)
		fclose(file);

	return res;
}

bool_t
clish_shell_push_fd(clish_shell_t * this, FILE * file, bool_t stop_on_error)
{
	assert(this);

	/* allocate a control node */
	clish_shell_file_t *node = malloc(sizeof(clish_shell_file_t));
	bool_t result = BOOL_TRUE;

	if (NULL != node) {
		/* intialise the node */
		node->file = file;
		node->stop_on_error = stop_on_error;
		node->next = this->current_file;

		/* put the node at the top of the file stack */
		this->current_file = node;

		/* now switch the terminal's input stream */
		tinyrl__set_istream(this->tinyrl, file);

		result = BOOL_TRUE;
	}
	return result;
}
