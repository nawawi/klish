/*
 * string_escape.c
 */
#include "private.h"

#include <stdlib.h>
#include <string.h>

/*
 * These are the escape characters which are used by default when 
 * expanding variables. These characters will be backslash escaped
 * to prevent them from being interpreted in a script.
 *
 * This is a security feature to prevent users from arbitarily setting
 * parameters to contain special sequences.
 */
static const char *default_escape_chars = "`|$<>&()#";

/*--------------------------------------------------------- */
char *
lub_string_decode(const char *string)
{
    const char *s = string;
    char *res, *p;
    int esc = 0;

    if (!string)
        return NULL;

    /* Allocate enough memory for result */
    p = res = malloc(strlen(string) + 1);

    while (*s) {
        if (!esc) {
            if ('\\' == *s)
                esc = 1;
            else
                *p = *s;
        } else {
            switch (*s) {
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
            esc = 0;
        }
        if (!esc)
            p++;
        s++;
    }
    *p = '\0';

    /* Optimize the memory allocated for result */
    p = lub_string_dup(res);
    free(res);

    return p;
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

	if (NULL == escape_chars) {
		escape_chars = default_escape_chars;
	}
	for (p = string; p && *p; p++)
	{
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
