/*
 * argv_nextword.c
 */
#include <stddef.h>
#include <ctype.h>

#include "private.h"
#include "lub/types.h"

/*--------------------------------------------------------- */
const char *lub_argv_nextword(const char *string,
	size_t * len, size_t * offset, bool_t * quoted)
{
	const char *word;
	bool_t quote = BOOL_FALSE;

	*quoted = BOOL_FALSE;

	/* find the start of a word (not including an opening quote) */
	while (*string && isspace(*string)) {
		string++;
		(*offset)++;
	}
	if (*string == '\\') {
		string++;
		if (*string)
			string++;
	}
	/* is this the start of a quoted string ? */
	if (*string == '"') {
		quote = BOOL_TRUE;
		string++;
	}
	word = string;
	*len = 0;

	/* find the end of the word */
	while (*string) {
		if (*string == '\\') {
			string++;
			(*len)++;
			if (*string) {
				(*len)++;
				string++;
			}
			continue;
		}
		if ((BOOL_FALSE == quote) && isspace(*string)) {
			/* end of word */
			break;
		}
		if (*string == '"') {
			/* end of a quoted string */
			*quoted = BOOL_TRUE;
			break;
		}
		(*len)++;
		string++;
	}
	return word;
}

/*--------------------------------------------------------- */
unsigned lub_argv_wordcount(const char *line)
{
	const char *word;
	unsigned result = 0;
	size_t len = 0, offset = 0;
	bool_t quoted;

	for (word = lub_argv_nextword(line, &len, &offset, &quoted);
		*word;
		word = lub_argv_nextword(word + len, &len, &offset, &quoted)) {
		/* account for the terminating quotation mark */
		len += (BOOL_TRUE == quoted) ? 1 : 0;
		result++;
	}

	return result;
}

/*--------------------------------------------------------- */
