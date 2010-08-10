/*
 * argv_nextword.c
 */
#include "private.h"
#include "lub/types.h"
#include "lub/ctype.h"

#include <stddef.h>
/*--------------------------------------------------------- */
const char *
lub_argv_nextword(const char *string,
                  size_t     *len,
                  size_t     *offset,
                  bool_t     *quoted)
{
    const char *word;
    bool_t      quote = BOOL_FALSE;
    
    *quoted = BOOL_FALSE;
    
    /* find the start of a word (not including an opening quote) */
    while(*string && lub_ctype_isspace(*string))
    {
        string++;
        (*offset)++;
    }
    /* is this the start of a quoted string ? */
    if(*string == '"')
    {
        quote = BOOL_TRUE;
        string++;
    }
    word = string;
    *len = 0;
    
    /* find the end of the word */
    while(*string)
    {
        if((BOOL_FALSE == quote) && lub_ctype_isspace(*string))
        {
            /* end of word */
            break;
        }
        if(*string == '"')
        {
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
