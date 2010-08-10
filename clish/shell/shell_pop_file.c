#include <stdlib.h>
#include <stdio.h>

#include "private.h"

bool_t clish_shell_pop_file(clish_shell_t * this)
{
	bool_t result = BOOL_FALSE;
	clish_shell_file_t *node = this->current_file;

	if (node) {

		/* remove the current file from the stack... */
		this->current_file = node->next;

		/* and close the current file...
		 */
		fclose(node->file);

		if (node->next) {
			/* now switch the terminal's input stream */
			tinyrl__set_istream(this->tinyrl, node->next->file);
			result = BOOL_TRUE;
		}
		/* and free up the memory */
		free(node);
	}
	return result;
}
