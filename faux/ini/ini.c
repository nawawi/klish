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
#include "faux/file.h"
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
const faux_pair_t *faux_ini_set(
	faux_ini_t *ini, const char *name, const char *value) {

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
		faux_pair_set_value(
			found_pair, value); // Replace value by new one
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
faux_ini_node_t *faux_ini_iter(const faux_ini_t *ini) {

	assert(ini);
	if (!ini)
		return NULL;

	return (faux_ini_node_t *)faux_list_head(ini->list);
}


/** @brief Iterate entire INI object for pairs 'name/value'.
 *
 * Before iteration the iterator must be initialized by faux_ini_iter()
 * function. Doesn't use faux_ini_each() with uninitialized iterator.
 *
 * On each call function returns pair 'name/value' and modify iterator.
 * Stop iteration when function returns NULL.
 *
 * @param [in,out] iter Iterator.
 * @return Pair 'name/value'.
 * @sa faux_ini_iter()
 */
const faux_pair_t *faux_ini_each(faux_ini_node_t **iter) {

	return (const faux_pair_t *)faux_list_each((faux_list_node_t **)iter);
}


/** Service function to purify (clean out spaces, quotes) word.
 *
 * The 'word' in this case is a string without prepending or trailing spaces.
 * If 'word' is quoted then it can contain spaces within quoting. The qoutes
 * itself is not part of the 'word'. If 'word' is not quoted then it can't
 * contain spaces, so the end of 'word' is a first space after non-space
 * characters. The function searchs for the first occurence of 'word' within
 * specified string, allocates memory and returns purified copy of the word.
 * The return value must be faux_str_free()-ed later.
 *
 * Now the unclosed quoting is not an error. Suppose the end of the line can
 * close quoting.
 *
 * @param [in] str String to find word in it.
 * @return Purified copy of word or NULL.
 */
static char *faux_ini_purify_word(const char *str) {
	const char *word;
	const char *string = str;
	bool_t quoted = BOOL_FALSE;
	size_t len = 0;

	assert(str);
	if (!str)
		return NULL;

	// Find the start of a word
	while (*string != '\0' && isspace(*string)) {
		string++;
	}
	// Is this the start of a quoted string?
	if ('"' == *string) {
		quoted = BOOL_TRUE;
		string++;
	}
	word = string; // Begin of purified word

	// Find the end of the word
	while (*string != '\0') {
		if ('\\' == *string) {
			string++;
			if ('\0' == *string) // Unfinished escaping
				break; // Don't increment 'len'
			len++;
			// Skip escaped char
			string++;
			len++;
			continue;
		}
		// End of word
		if (!quoted && isspace(*string))
			break;
		if ('"' == *string) {
			// End of a quoted string
			break;
		}
		string++;
		len++;
	}

	if (len == 0)
		return NULL;

	return faux_str_dupn(word, len);
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
	for (line = strtok_r(buffer, "\n\r", &saveptr); line;
		line = strtok_r(NULL, "\n\r", &saveptr)) {

		// Now 'line' contain one 'name/value' pair. Single line.
		char *str = NULL;
		char *name = NULL;
		char *value = NULL;
		char *savestr = NULL;
		char *rname = NULL;
		char *rvalue = NULL;

		while ((*line != '\0') && isspace(*line)) // Skip spaces
			line++;
		if ('\0' == *line) // Empty line
			continue;
		if ('#' == *line) // Comment. Skip it.
			continue;
		str = faux_str_dup(line);

		// Find out name
		name = strtok_r(str, "=", &savestr);
		if (!name) {
			faux_str_free(str);
			continue;
		}
		rname = faux_ini_purify_word(name);
		if (!rname) {
			faux_str_free(str);
			continue;
		}

		// Find out value
		value = strtok_r(NULL, "=", &savestr);
		if (!value) { // Empty value
			rvalue = NULL;
		} else {
			rvalue = faux_ini_purify_word(value);
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

	bool_t eof = BOOL_FALSE;
	faux_file_t *f = NULL;
	char *buf = NULL;

	assert(ini);
	assert(fn);
	if (!ini)
		return -1;
	if (!fn || '\0' == *fn)
		return -1;

	f = faux_file_open(fn, O_RDONLY, 0);
	if (!f)
		return -1;

	while ((buf = faux_file_getline(f))) {
		// Don't analyze retval because it's not obvious what
		// to do on error. May be next string will be ok.
		faux_ini_parse_str(ini, buf);
		faux_str_free(buf);
	}

	eof = faux_file_eof(f);
	faux_file_close(f);
	if (!eof) // File reading was interrupted before EOF
		return -1;

	return 0;
}


/** Writes INI file using INI object.
 *
 * Write pairs 'name/value' to INI file. The source of pairs is an INI object.
 * It's complementary operation to faux_ini_parse_file().
 *
 * @param [in] ini Allocated and initialized INI object.
 * @param [in] fn File name to write to.
 * @return 0 - success, < 0 - error
 */
int faux_ini_write_file(const faux_ini_t *ini, const char *fn) {

	faux_file_t *f = NULL;
	faux_ini_node_t *iter = NULL;
	const faux_pair_t *pair = NULL;
	const char *spaces = " \t"; // String with spaces needs quotes

	assert(ini);
	assert(fn);
	if (!ini)
		return -1;
	if (!fn || '\0' == *fn)
		return -1;

	f = faux_file_open(fn, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (!f)
		return -1;

	iter = faux_ini_iter(ini);
	while ((pair = faux_ini_each(&iter))) {
		char *quote_name = NULL;
		char *quote_value = NULL;
		const char *name = faux_pair_name(pair);
		const char *value = faux_pair_value(pair);
		char *line = NULL;
		ssize_t bytes_written = 0;

		// Word with spaces needs quotes
		quote_name = faux_str_chars(name, spaces) ? "\"" : "";
		quote_value = faux_str_chars(value, spaces) ? "\"" : "";

		// Prepare INI line
		line = faux_str_sprintf("%s%s%s=%s%s%s\n",
			quote_name, name, quote_name,
			quote_value, value, quote_value);
		if (!line) {
			faux_file_close(f);
			return -1;
		}

		// Write to file
		bytes_written = faux_file_write(f, line, strlen(line));
		faux_str_free(line);
		if (bytes_written < 0) { // Can't write to file
			faux_file_close(f);
			return -1;
		}
	}

	faux_file_close(f);

	return 0;
}
