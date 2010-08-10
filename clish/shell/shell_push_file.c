#include <stdlib.h>

#include "private.h"

bool_t
clish_shell_push_file(clish_shell_t * this, FILE * file, bool_t stop_on_error)
{
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
