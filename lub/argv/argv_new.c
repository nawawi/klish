/*
 * argv_new.c
 */
#include "private.h"
#include "lub/string.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

/*--------------------------------------------------------- */
static void lub_argv_init(lub_argv_t * this, const char *line, size_t offset)
{
	size_t len;
	const char *word;
	lub_arg_t *arg;
	bool_t quoted;

	if (!line) {
		this->argv = NULL;
		this->argc = 0;
		return;
	}

	/* first of all count the words in the line */
	this->argc = lub_argv_wordcount(line);

	/* allocate space to hold the vector */
	arg = this->argv = malloc(sizeof(lub_arg_t) * this->argc);
		assert(arg);

	/* then fill out the array with the words */
	for (word = lub_argv_nextword(line, &len, &offset, &quoted);
		*word;
		word = lub_argv_nextword(word + len, &len, &offset, &quoted)) {
		char *tmp = lub_string_dupn(word, len);
		(*arg).arg = lub_string_decode(tmp);
		lub_string_free(tmp);
		(*arg).offset = offset;
		(*arg).quoted = quoted;

		offset += len;

		if (quoted) {
			len += 1; /* account for terminating quotation mark */
			offset += 2; /* account for quotation marks */
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
	(this->argv[this->argc++]).arg = lub_string_dup(text);
}

/*--------------------------------------------------------- */
