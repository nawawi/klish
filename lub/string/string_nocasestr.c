/*
 * string_nocasestr.c
 *
 * Find a string within another string in a case insensitive manner
 */
 #include "private.h"
 
 #include "lub/ctype.h"
 
/*--------------------------------------------------------- */
const char *
lub_string_nocasestr(const char *cs,
                     const char *ct)
{
    const char *p      = NULL;
    const char *result = NULL;
    
    while(*cs)
    {
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
        while(*p && *q && (lub_ctype_tolower(*p) == lub_ctype_tolower(*q)))
        {
            p++,q++;
        }
        if(0 == *p)
        { 
            break;
        }
        cs++;
    }
    if(p && !*p)
    {
        /* we've found the first match of ct within cs */
        result = cs;
    }
    return result;
}
/*--------------------------------------------------------- */
