/** @file klevel.c
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <faux/str.h>
#include <klish/khelper.h>
#include <klish/kpath.h>
#include <klish/kview.h>

struct klevel_s {
	kview_t *view;
};


// View
KGET(level, kview_t *, view);


klevel_t *klevel_new(kview_t *view)
{
	klevel_t *level = NULL;

	if (!view)
		return NULL;

	level = faux_zmalloc(sizeof(*level));
	assert(level);
	if (!level)
		return NULL;

	// Initialize
	level->view = view;

	return level;
}


void klevel_free(klevel_t *level)
{
	if (!level)
		return;

	faux_free(level);
}
