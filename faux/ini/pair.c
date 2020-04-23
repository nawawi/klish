/** @file pair.c
 * Ini file pairs key/value.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "private.h"
#include "faux/str.h"
#include "faux/ini.h"

int faux_pair_compare(const void *first, const void *second) {

	const faux_pair_t *f = (const faux_pair_t *)first;
	const faux_pair_t *s = (const faux_pair_t *)second;

	return strcmp(f->name, s->name);
}


int faux_pair_kcompare(const void *key, const void *list_item) {

	const char *f = (const char *)key;
	const faux_pair_t *s = (const faux_pair_t *)list_item;

	return strcmp(f, s->name);
}


faux_pair_t *faux_pair_new(const char *name, const char *value) {

	faux_pair_t *pair = NULL;

	pair = faux_zmalloc(sizeof(*pair));
	assert(pair);
	if (!pair)
		return NULL;

	// Initialize
	pair->name = faux_str_dup(name);
	pair->value = faux_str_dup(value);

	return pair;
}


void faux_pair_free(void *ptr) {

	faux_pair_t *pair = (faux_pair_t *)ptr;

	assert(pair);
	if (!pair)
		return;
	faux_str_free(pair->name);
	faux_str_free(pair->value);
	faux_free(pair);
}


const char *faux_pair_name(const faux_pair_t *pair) {

	assert(pair);
	if (!pair)
		return NULL;

	return pair->name;
}


void faux_pair_set_name(faux_pair_t *pair, const char *name) {

	assert(pair);
	if (!pair)
		return;

	faux_str_free(pair->name);
	pair->name = faux_str_dup(name);
}


const char *faux_pair_value(const faux_pair_t *pair) {

	assert(pair);
	if (!pair)
		return NULL;

	return pair->value;
}


void faux_pair_set_value(faux_pair_t *pair, const char *value) {

	assert(pair);
	if (!pair)
		return;

	faux_str_free(pair->value);
	pair->value = faux_str_dup(value);
}
