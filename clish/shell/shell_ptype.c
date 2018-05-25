/*
 * shell_find_create_ptype.c
 */

#include <string.h>
#include <assert.h>

#include "private.h"

/*--------------------------------------------------------- */
clish_ptype_t *clish_shell_find_ptype(clish_shell_t *this, const char *name)
{
	lub_list_node_t *iter;

	assert(this);

	if (!name || !name[0])
		return NULL;
	/* Iterate elements */
	for(iter = lub_list__get_head(this->ptype_tree);
		iter; iter = lub_list_node__get_next(iter)) {
		int r;
		clish_ptype_t *ptype = (clish_ptype_t *)lub_list_node__get_data(iter);
		r = strcmp(name, clish_ptype__get_name(ptype));
		if (!r)
			return ptype;
		if (r < 0)
			break;
	}

	return NULL;
}

/*--------------------------------------------------------- */
clish_ptype_t *clish_shell_find_create_ptype(clish_shell_t * this,
	const char *name, const char *text, const char *pattern,
	clish_ptype_method_e method, clish_ptype_preprocess_e preprocess)
{
	clish_ptype_t *ptype = clish_shell_find_ptype(this, name);

	if (!ptype) {
		/* Create a ptype */
		ptype = clish_ptype_new(name, text, pattern,
			method, preprocess);
		assert(ptype);
		lub_list_add(this->ptype_tree, ptype);
	}

	return ptype;
}
