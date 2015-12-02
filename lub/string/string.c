/*
 * string.c
 */
#include "private.h"

#include <stdlib.h>
#include <string.h>

#include "lub/ctype.h"

const char *lub_string_esc_default = "`|$<>&()#;\\\"!";
const char *lub_string_esc_regex = "^$.*+[](){}";
const char *lub_string_esc_quoted = "\\\"";

/*--------------------------------------------------------- */
void lub_string_free(char *ptr)
{
	free(ptr);
}

/*--------------------------------------------------------- */
char *lub_string_ndecode(const char *string, unsigned int len)
{
	const char *s = string;
	char *res, *p;
	int esc = 0;

	if (!string)
		return NULL;

	/* Allocate enough memory for result */
	p = res = malloc(len + 1);

	while (*s && (s < (string +len))) {
		if (!esc) {
			if ('\\' == *s)
				esc = 1;
			else
				*p = *s;
		} else {
/*			switch (*s) {
			case 'r':
			case 'n':
				*p = '\n';
				break;
			case 't':
				*p = '\t';
				break;
			default:
				*p = *s;
				break;
			}
*/			*p = *s;
			esc = 0;
		}
		if (!esc)
			p++;
		s++;
	}
	*p = '\0';

	return res;
}

/*--------------------------------------------------------- */
inline char *lub_string_decode(const char *string)
{
	return lub_string_ndecode(string, strlen(string));
}

/*----------------------------------------------------------- */
/*
 * This needs to escape any dangerous characters within the command line
 * to prevent gaining access to the underlying system shell.
 */
char *lub_string_encode(const char *string, const char *escape_chars)
{
	char *result = NULL;
	const char *p;

	if (!escape_chars)
		return lub_string_dup(string);
	if (string && !(*string)) /* Empty string */
		return lub_string_dup(string);

	for (p = string; p && *p; p++) {
		/* find any special characters and prefix them with '\' */
		size_t len = strcspn(p, escape_chars);
		lub_string_catn(&result, p, len);
		p += len;
		if (*p) {
			lub_string_catn(&result, "\\", 1);
			lub_string_catn(&result, p, 1);
		} else {
			break;
		}
	}
	return result;
}

/*--------------------------------------------------------- */
void lub_string_catn(char **string, const char *text, size_t len)
{
	if (text) {
		char *q;
		size_t length, initlen, textlen = strlen(text);

		/* make sure the client cannot give us duff details */
		len = (len < textlen) ? len : textlen;

		/* remember the size of the original string */
		initlen = *string ? strlen(*string) : 0;

		/* account for '\0' */
		length = initlen + len + 1;

		/* allocate the memory for the result */
		q = realloc(*string, length);
		if (NULL != q) {
			*string = q;
			/* move to the end of the initial string */
			q += initlen;

			while (len--) {
				*q++ = *text++;
			}
			*q = '\0';
		}
	}
}

/*--------------------------------------------------------- */
void lub_string_cat(char **string, const char *text)
{
	size_t len = text ? strlen(text) : 0;
	lub_string_catn(string, text, len);
}

/*--------------------------------------------------------- */
char *lub_string_dup(const char *string)
{
	if (!string)
		return NULL;
	return strdup(string);
}

/*--------------------------------------------------------- */
char *lub_string_dupn(const char *string, unsigned int len)
{
	char *res = NULL;

	if (!string)
		return res;
	res = malloc(len + 1);
	strncpy(res, string, len);
	res[len] = '\0';

	return res;
}

/*--------------------------------------------------------- */
int lub_string_nocasecmp(const char *cs, const char *ct)
{
	int result = 0;
	while ((0 == result) && *cs && *ct) {
		/*lint -e155 Ignoring { }'ed sequence within an expression, 0 assumed 
		 * MACRO implementation uses braces to prevent multiple increments
		 * when called.
		 */
		int s = lub_ctype_tolower(*cs++);
		int t = lub_ctype_tolower(*ct++);

		result = s - t;
	}
	/*lint -e774 Boolean within 'if' always evealuates to True 
	 * not the case because of tolower() evaluating to 0 under lint
	 * (see above)
	 */
	if (0 == result) {
		/* account for different string lengths */
		result = *cs - *ct;
	}
	return result;
}

/*--------------------------------------------------------- */
char *lub_string_tolower(const char *str)
{
	char *tmp = strdup(str);
	char *p = tmp;

	while (*p) {
		*p = tolower(*p);
		p++;
	}

	return tmp;
}

/*--------------------------------------------------------- */
const char *lub_string_nocasestr(const char *cs, const char *ct)
{
	const char *p = NULL;
	const char *result = NULL;

	while (*cs) {
		const char *q = cs;

		p = ct;
		/*lint -e155 Ignoring { }'ed sequence within an expression, 0 assumed 
		 * MACRO implementation uses braces to prevent multiple increments
		 * when called.
		 */
		/*lint -e506 Constant value Boolean
		 * not the case because of tolower() evaluating to 0 under lint
		 * (see above)
		 */
		while (*p && *q
		       && (lub_ctype_tolower(*p) == lub_ctype_tolower(*q))) {
			p++, q++;
		}
		if (0 == *p) {
			break;
		}
		cs++;
	}
	if (p && !*p) {
		/* we've found the first match of ct within cs */
		result = cs;
	}
	return result;
}

/*--------------------------------------------------------- */
unsigned int lub_string_equal_part(const char *str1, const char *str2,
	bool_t utf8)
{
	unsigned int cnt = 0;

	if (!str1 || !str2)
		return cnt;
	while (*str1 && *str2) {
		if (*str1 != *str2)
			break;
		cnt++;
		str1++;
		str2++;
	}
	if (!utf8)
		return cnt;

	/* UTF8 features */
	if (cnt && (UTF8_11 == (*(str1 - 1) & UTF8_MASK)))
		cnt--;

	return cnt;
}

/*--------------------------------------------------------- */
const char *lub_string_suffix(const char *string)
{
	const char *p1, *p2;
	p1 = p2 = string;
	while (*p1) {
		if (lub_ctype_isspace(*p1)) {
			p2 = p1;
			p2++;
		}
		p1++;
	}
	return p2;
}

/*--------------------------------------------------------- */
const char *lub_string_nextword(const char *string,
	size_t *len, size_t *offset, size_t *quoted)
{
	const char *word;

	*quoted = 0;

	/* Find the start of a word (not including an opening quote) */
	while (*string && isspace(*string)) {
		string++;
		(*offset)++;
	}
	/* Is this the start of a quoted string ? */
	if (*string == '"') {
		*quoted = 1;
		string++;
	}
	word = string;
	*len = 0;

	/* Find the end of the word */
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
		/* End of word */
		if (!*quoted && isspace(*string))
			break;
		if (*string == '"') {
			/* End of a quoted string */
			*quoted = 2;
			break;
		}
		(*len)++;
		string++;
	}

	return word;
}

/*--------------------------------------------------------- */
unsigned int lub_string_wordcount(const char *line)
{
	const char *word;
	unsigned int result = 0;
	size_t len = 0, offset = 0;
	size_t quoted;

	for (word = lub_string_nextword(line, &len, &offset, &quoted);
		*word || quoted;
		word = lub_string_nextword(word + len, &len, &offset, &quoted)) {
		/* account for the terminating quotation mark */
		len += quoted ? quoted - 1 : 0;
		result++;
	}

	return result;
}

/*--------------------------------------------------------- */
