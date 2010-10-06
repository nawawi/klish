/*
 * string_escape.c
 */
#include "private.h"

#include <stdlib.h>
#include <string.h>
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

/*--------------------------------------------------------- */
