/*
 * argv.c
 */
#include "private.h"
#include "lub/string.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

/*--------------------------------------------------------- */
static void lub_argv_init(lub_argv_t * this, const char *line, size_t offset)
{
	size_t len;
	const char *word;
	lub_arg_t *arg;
	size_t quoted;

	this->argv = NULL;
	this->argc = 0;
	if (!line)
		return;
	/* first of all count the words in the line */
	this->argc = lub_string_wordcount(line);
	if (0 == this->argc)
		return;
	/* allocate space to hold the vector */
	arg = this->argv = malloc(sizeof(lub_arg_t) * this->argc);
	assert(arg);

	/* then fill out the array with the words */
	for (word = lub_string_nextword(line, &len, &offset, &quoted);
		*word || quoted;
		word = lub_string_nextword(word + len, &len, &offset, &quoted)) {
		(*arg).arg = lub_string_ndecode(word, len);
		(*arg).offset = offset;
		(*arg).quoted = quoted ? BOOL_TRUE : BOOL_FALSE;

		offset += len;

		if (quoted) {
			len += quoted - 1; /* account for terminating quotation mark */
			offset += quoted; /* account for quotation marks */
		}
		arg++;
	}
}

/*--------------------------------------------------------- */
lub_argv_t *lub_argv_new(const char *line, size_t offset)
{
	lub_argv_t *this;

	this = malloc(sizeof(lub_argv_t));
	if (this)
		lub_argv_init(this, line, offset);

	return this;
}

/*--------------------------------------------------------- */
void lub_argv_add(lub_argv_t * this, const char *text)
{
	lub_arg_t * arg;

	if (!text)
		return;

	/* allocate space to hold the vector */
	arg = realloc(this->argv, sizeof(lub_arg_t) * (this->argc + 1));
	assert(arg);
	this->argv = arg;
	(this->argv[this->argc++]).arg = strdup(text);
}

/*--------------------------------------------------------- */
static void lub_argv_fini(lub_argv_t * this)
{
	unsigned i;

	for (i = 0; i < this->argc; i++)
		free(this->argv[i].arg);
	free(this->argv);
	this->argv = NULL;
}

/*--------------------------------------------------------- */
void lub_argv_delete(lub_argv_t * this)
{
	lub_argv_fini(this);
	free(this);
}

/*--------------------------------------------------------- */
char *lub_argv__get_line(const lub_argv_t * this)
{
	int space = 0;
	const char *p;
	unsigned i;
	char *line = NULL;

	for (i = 0; i < this->argc; i++) {
		if (i != 0)
			lub_string_cat(&line, " ");
		space = 0;
		/* Search for spaces */
		for (p = this->argv[i].arg; *p; p++) {
			if (isspace(*p)) {
				space = 1;
				break;
			}
		}
		if (space)
			lub_string_cat(&line, "\"");
		lub_string_cat(&line, this->argv[i].arg);
		if (space)
			lub_string_cat(&line, "\"");
	}

	return line;
}

/*--------------------------------------------------------- */
char **lub_argv__get_argv(const lub_argv_t * this, const char *argv0)
{
	char **result = NULL;
	unsigned i;
	unsigned a = 0;

	if (argv0)
		a = 1;

	result = malloc(sizeof(char *) * (this->argc + 1 + a));

	if (argv0)
		result[0] = strdup(argv0);
	for (i = 0; i < this->argc; i++)
		result[i + a] = strdup(this->argv[i].arg);
	result[i + a] = NULL;

	return result;
}

/*--------------------------------------------------------- */
void lub_argv__free_argv(char **argv)
{
	unsigned i;

	if (!argv)
		return;

	for (i = 0; argv[i]; i++)
		free(argv[i]);
	free(argv);
}

/*--------------------------------------------------------- */
const char *lub_argv__get_arg(const lub_argv_t *this, unsigned int index)
{
	const char *result = NULL;

	if (!this)
		return NULL;
	if (this->argc > index)
		result = this->argv[index].arg;

	return result;
}

/*--------------------------------------------------------- */
unsigned lub_argv__get_count(const lub_argv_t * this)
{
	return this->argc;
}

/*--------------------------------------------------------- */
size_t lub_argv__get_offset(const lub_argv_t * this, unsigned index)
{
	size_t result = 0;

	if (this->argc > index)
		result = this->argv[index].offset;

	return result;
}

/*--------------------------------------------------------- */
bool_t lub_argv__get_quoted(const lub_argv_t * this, unsigned index)
{
	bool_t result = BOOL_FALSE;

	if (this->argc > index)
		result = this->argv[index].quoted;

	return result;
}

/*--------------------------------------------------------- */
