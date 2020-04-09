/** @file ini.c
 * @brief Functions for working with INI files.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include "private.h"
#include "faux/faux.h"
#include "faux/str.h"
#include "faux/ini.h"


faux_ini_t *faux_ini_new(void) {

	faux_ini_t *ini;

	ini = faux_zmalloc(sizeof(*ini));
	if (!ini)
		return NULL;

	// Init
	ini->list = faux_list_new(faux_pair_compare, faux_pair_free);

	return ini;
}


void faux_ini_free(faux_ini_t *ini) {

	assert(ini);
	if (!ini)
		return;

	faux_list_free(ini->list);
	faux_free(ini);
}


static int faux_ini_del(faux_ini_t *ini, faux_ini_node_t *node) {

	assert(ini);
	assert(node);
	if (!ini || !node)
		return -1;

	return faux_list_del(ini->list, (faux_list_node_t *)node);
}

faux_pair_t *faux_ini_set(faux_ini_t *ini, const char *name, const char *value) {

	faux_pair_t *pair = NULL;
	faux_list_node_t *node = NULL;
	faux_pair_t *found_pair = NULL;

	assert(ini);
	assert(name);
	if (!ini || !name)
		return NULL;

	pair = faux_pair_new(name, value);
	assert(pair);
	if (!pair)
		return NULL;

	// NULL 'value' means: remove entry from list
	if (!value) {
		node = faux_list_find_node(ini->list, faux_pair_compare, pair);
		faux_pair_free(pair);
		if (node)
			faux_list_del(ini->list, node);
		return NULL;
	}

	// Try to add new entry or find existent entry with the same 'name'
	node = faux_list_add_find(ini->list, pair);
	if (!node) { // Something went wrong
		faux_pair_free(pair);
		return NULL;
	}
	found_pair = faux_list_data(node);
	if (found_pair != pair) { // Item already exists so use existent
		faux_pair_free(pair);
		faux_pair_set_value(found_pair, value); // Replace value by new one
		return found_pair;
	}

	// The new entry was added
	return pair;
}


void faux_ini_unset(faux_ini_t *ini, const char *name) {

	faux_ini_set(ini, name, NULL);
}


/* Find pair by name */
faux_pair_t *faux_ini_find_pair(const faux_ini_t *ini, const char *name) {

	faux_list_node_t *iter = NULL;
	faux_pair_t *pair = NULL;

	assert(ini);
	assert(name);
	if (!ini || !name)
		return NULL;

	pair = faux_pair_new(name, NULL);
	if (!pair)
		return NULL;
	iter = faux_list_find_node(ini->list, faux_pair_compare, pair);
	faux_pair_free(pair);

	return faux_list_data(iter);
}


/* Find value by name */
const char *faux_ini_find(const faux_ini_t *ini, const char *name) {

	faux_pair_t *pair = faux_ini_find_pair(ini, name);

	if (!pair)
		return NULL;

	return faux_pair_value(pair);
}


int faux_ini_parse_str(faux_ini_t *ini, const char *string) {

	char *buffer = NULL;
	char *saveptr = NULL;
	char *line = NULL;

	assert(ini);
	if (!ini)
		return -1;
	if (!string)
		return 0;

	buffer = faux_str_dup(string);
	// Now loop though each line
	for (line = strtok_r(buffer, "\n", &saveptr);
		line; line = strtok_r(NULL, "\n", &saveptr)) {

		char *str = NULL;
		char *name = NULL;
		char *value = NULL;
		char *savestr = NULL;
		char *ns = line;
		const char *begin = NULL;
		size_t len = 0;
		size_t offset = 0;
		size_t quoted = 0;
		char *rname = NULL;
		char *rvalue = NULL;

		if (!*ns) // Empty
			continue;
		while (*ns && isspace(*ns))
			ns++;
		if ('#' == *ns) // Comment
			continue;
		if ('=' == *ns) // Broken string
			continue;
		str = faux_str_dup(ns);
		name = strtok_r(str, "=", &savestr);
		if (!name) {
			faux_str_free(str);
			continue;
		}
		value = strtok_r(NULL, "=", &savestr);
		begin = faux_str_nextword(name, &len, &offset, &quoted);
		rname = faux_str_dupn(begin, len);
		if (!value) { // Empty value
			rvalue = NULL;
		} else {
			begin = faux_str_nextword(value, &len, &offset, &quoted);
			rvalue = faux_str_dupn(begin, len);
		}
		faux_ini_set(ini, rname, rvalue);
		faux_str_free(rname);
		faux_str_free(rvalue);
		faux_str_free(str);
	}
	faux_str_free(buffer);

	return 0;
}


int faux_ini_parse_file(faux_ini_t *ini, const char *fn) {

	int ret = -1;
	FILE *f = NULL;
	char *buf = NULL;
	unsigned int p = 0;
	const int chunk_size = 128;
	int size = chunk_size;

	assert(ini);
	assert(fn);
	if (!ini)
		return -1;
	if (!fn || !*fn)
		return -1;
	f = fopen(fn, "r");
	if (!f)
		return -1;

	buf = faux_zmalloc(size);
	while (fgets(buf + p, size - p, f)) {
		char *tmp = NULL;

		if (feof(f) || strchr(buf + p, '\n') || strchr(buf + p, '\r')) {
			faux_ini_parse_str(ini, buf);
			p = 0;
			continue;
		}
		p = size - 1;
		size += chunk_size;
		tmp = realloc(buf, size);
		if (!tmp)
			goto error;
		buf = tmp;
	}

	ret = 0;
error:
	faux_free(buf);
	fclose(f);

	return ret;
}


faux_ini_node_t *faux_ini_head(const faux_ini_t *ini) {

	assert(ini);
	if (!ini)
		return NULL;

	return faux_list_head(ini->list);
}


faux_ini_node_t *faux_ini_tail(const faux_ini_t *ini) {

	assert(ini);
	if (!ini)
		return NULL;

	return faux_list_tail(ini->list);
}


faux_ini_node_t *faux_ini_next(const faux_ini_node_t *node) {

	assert(node);
	if (!node)
		return NULL;

	return faux_list_next_node(node);
}


faux_ini_node_t *faux_ini_prev(const faux_ini_node_t *node) {

	assert(node);
	if (!node)
		return NULL;

	return faux_list_prev_node(node);
}


faux_pair_t *faux_ini_data(const faux_ini_node_t *node) {

	assert(node);
	if (!node)
		return NULL;

	return (faux_pair_t *)faux_list_data(node);
}
