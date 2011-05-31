#include <stdlib.h>
#include <assert.h>

#include "private.h"

/*----------------------------------------------------------- */
int clish_shell_push_file(clish_shell_t * this, const char * fname,
	bool_t stop_on_error)
{
	FILE *file;
	int res;

	assert(this);
	if (!fname)
		return -1;
	file = fopen(fname, "r");
	if (!file)
		return -1;
	res = clish_shell_push_fd(this, file, stop_on_error);
	if (res)
		fclose(file);

	return res;
}

/*----------------------------------------------------------- */
int clish_shell_push_fd(clish_shell_t * this, FILE * file,
	bool_t stop_on_error)
{
	/* Allocate a control node */
	clish_shell_file_t *node = malloc(sizeof(clish_shell_file_t));

	assert(this);
	assert(node);

	/* intialise the node */
	node->file = file;
	node->stop_on_error = stop_on_error;
	node->next = this->current_file;

	/* put the node at the top of the file stack */
	this->current_file = node;

	/* now switch the terminal's input stream */
	tinyrl__set_istream(this->tinyrl, file);

	return 0;
}

/*----------------------------------------------------------- */
int clish_shell_pop_file(clish_shell_t *this)
{
	int result = -1;
	clish_shell_file_t *node = this->current_file;

	if (!node)
		return -1;

	/* remove the current file from the stack... */
	this->current_file = node->next;
	/* and close the current file... */
	fclose(node->file);
	if (node->next) {
		/* now switch the terminal's input stream */
		tinyrl__set_istream(this->tinyrl, node->next->file);
		result = 0;
	}
	/* and free up the memory */
	free(node);

	return result;
}

/*----------------------------------------------------------- */
