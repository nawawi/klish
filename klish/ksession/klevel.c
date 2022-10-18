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


klevel_t *klevel_clone(const klevel_t *level)
{
	klevel_t *new_level = NULL;

	assert(level);
	if (!level)
		return NULL;

	new_level = klevel_new(klevel_entry(level));

	return new_level;
}


bool_t klevel_is_equal(const klevel_t *f, const klevel_t *s)
{
	if (!f && !s)
		return BOOL_TRUE;
	if (!f || !s)
		return BOOL_FALSE;
	if (klevel_entry(f) == klevel_entry(s))
		return BOOL_TRUE;

	return BOOL_FALSE;
}
