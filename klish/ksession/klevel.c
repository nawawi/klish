/** @file klevel.c
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <faux/str.h>
#include <klish/khelper.h>
#include <klish/kpath.h>
#include <klish/kentry.h>

struct klevel_s {
	const kentry_t *entry;
};


// ENTRY
KGET(level, const kentry_t *, entry);


klevel_t *klevel_new(const kentry_t *entry)
{
	klevel_t *level = NULL;

	if (!entry)
		return NULL;

	level = faux_zmalloc(sizeof(*level));
	assert(level);
	if (!level)
		return NULL;

	// Initialize
	level->entry = entry;

	return level;
}


void klevel_free(klevel_t *level)
{
	if (!level)
		return;

	faux_free(level);
}
