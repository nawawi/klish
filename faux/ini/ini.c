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


/** @brief Allocates new INI object.
 *
 * Before working with INI object it must be allocated and initialized.
 *
 * @return Allocated and initialized INI object or NULL on error.
 */
faux_ini_t *faux_ini_new(void) {

	faux_ini_t *ini;

	ini = faux_zmalloc(sizeof(*ini));
	if (!ini)
		return NULL;

	// Init
	ini->list = faux_list_new(faux_pair_compare, faux_pair_free);

	return ini;
}


/** @brief Frees the INI object.
 *
 * After using the INI object must be freed. Function frees INI objecr itself
 * and all pairs 'name/value' stored within INI object.
 */
void faux_ini_free(faux_ini_t *ini) {

	assert(ini);
	if (!ini)
		return;

	faux_list_free(ini->list);
	faux_free(ini);
}


/** @brief Adds pair 'name/value' to INI object.
 *
 * The 'name' field is a key. The key must be unique. Each key has its
 * correspondent value.
 *
 * If key is new for the INI object then the pair 'name/value' will be added
 * to it. If INI object already contain the same key then value of this pair
 * will be replaced by newer one. If new specified value is NULL then the
 * entry with the correspondent key will be removed from the INI object.
 *
 * @param [in] ini Allocated and initialized INI object.
 * @param [in] name Name field for pair 'name/value'.
 * @param [in] value Value field for pair 'name/value'.
 * @return
 * Newly created pair object.
 * NULL if entry was removed (value == NULL)
 * NULL on error
 */
const faux_pair_t *faux_ini_set(faux_ini_t *ini, const char *name, const char *value) {

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


/** @brief Removes pair 'name/value' from INI object.
 *
 * Function search for the pair with specified name within INI object and
 * removes it.
 *
 * @param [in] ini Allocated and initialized INI object.
 * @param [in] name Name field to search for the entry.
 */
void faux_ini_unset(faux_ini_t *ini, const char *name) {

	faux_ini_set(ini, name, NULL);
}


/** @brief Searches for pair by name.
 *
 * The name field is a key to search INI object for pair 'name/value'.
 *
 * @param [in] ini Allocated and initialized INI object.
 * @param [in] name Name field to search for.
 * @return
 * Found pair 'name/value'.
 * NULL on errror.
 */
const faux_pair_t *faux_ini_find_pair(const faux_ini_t *ini, const char *name) {

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


/** @brief Searches for pair by name and returns correspondent value.
 *
 * The name field is a key to search INI object for pair 'name/value'.
 *
 * @param [in] ini Allocated and initialized INI object.
 * @param [in] name Name field to search for.
 * @return
 * Found value field.
 * NULL on errror.
 */
const char *faux_ini_find(const faux_ini_t *ini, const char *name) {

	const faux_pair_t *pair = faux_ini_find_pair(ini, name);

	if (!pair)
		return NULL;

	return faux_pair_value(pair);
}


/** @brief Initializes iterator to iterate through the entire INI object.
 *
 * Before iterating with the faux_ini_each() function the iterator must be
 * initialized. This function do it.
 *
 * @param [in] ini Allocated and initialized INI object.
 * @return Initialized iterator.
 * @sa faux_ini_each()
 */
faux_ini_node_t *faux_ini_init_iter(const faux_ini_t *ini) {

	assert(ini);
	if (!ini)
		return NULL;

	return (faux_ini_node_t *)faux_list_head(ini->list);
}


/** @brief Iterate entire INI object for pairs 'name/value'.
 *
 * Before iteration the iterator must be initialized by faux_ini_init_iter()
 * function. Doesn't use faux_ini_each() with uninitialized iterator.
 *
 * On each call function returns pair 'name/value' and modify iterator.
 * Stop iteration when function returns NULL.
 *
 * @param [in,out] iter Iterator.
 * @return Pair 'name/value'.
 * @sa faux_ini_init_iter()
 */
const faux_pair_t *faux_ini_each(faux_ini_node_t **iter) {

	return (const faux_pair_t *)faux_list_each((faux_list_node_t **)iter);
}


/** @brief Parse string for pairs 'name/value'.
 *
 * String can contain an `name/value` pairs in following format:
 * @code
 * var1=value1
 * var2 = "value 2"
 * @endcode
 * Function parses that string and stores 'name/value' pairs to
 * the INI object.
 *
 * @param [in] ini Allocated and initialized INI object.
 * @param [in] string String to parse.
 * @return 0 - succes, < 0 - error
 */
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


/** @brief Parse file for pairs 'name/value'.
 *
 * File can contain an `name/value` pairs in following format:
 * @code
 * var1=value1
 * var2 = "value 2"
 * @endcode
 * Function parses file and stores 'name/value' pairs to
 * the INI object.
 *
 * @param [in] ini Allocated and initialized INI object.
 * @param [in] string String to parse.
 * @return 0 - succes, < 0 - error
 * @sa faux_ini_parse_str()
 */
int faux_ini_parse_file(faux_ini_t *ini, const char *fn) {

	int ret = -1; // Pessimistic retval
	FILE *fd = NULL;
	char *buf = NULL;
	unsigned int bytes_readed = 0;
	const int chunk_size = 128;
	int size = chunk_size; // Buffer size

	assert(ini);
	assert(fn);
	if (!ini)
		return -1;
	if (!fn || '\0' == *fn)
		return -1;
	fd = fopen(fn, "r");
	if (!fd)
		return -1;

	buf = faux_zmalloc(size);
	while (fgets(buf + bytes_readed, size - bytes_readed, fd)) {

		// Not enough space in buffer. Make it larger.
		if (feof(fd) == 0 &&
			!strchr(buf + bytes_readed, '\n') &&
			!strchr(buf + bytes_readed, '\r')) {
			char *tmp = NULL;
			bytes_readed = size - 1; // fgets() put '\0' to last byte
			size += chunk_size;
			tmp = realloc(buf, size);
			if (!tmp) // Memory problems
				goto error;
			buf = tmp;
			continue; // Read the rest of line
		}

		// Don't analyze retval because it's not obvious what
		// to do on error. May be next string will be ok.
		faux_ini_parse_str(ini, buf);
		bytes_readed = 0;
	}

	ret = 0;
error:
	faux_free(buf);
	fclose(fd);

	return ret;
}