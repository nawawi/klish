/*
 * argv_wordcount.c
 *
 */
#include "private.h"

/*--------------------------------------------------------- */
unsigned
lub_argv_wordcount(const char *line)
{
    const char *word;
    unsigned    result=0;
    size_t      len=0,offset=0;
    bool_t      quoted;

    for(word = lub_argv_nextword(line,&len,&offset,&quoted);
        *word;
        word = lub_argv_nextword(word+len,&len,&offset,&quoted))
    {
        /* account for the terminating quotation mark */
        len += (BOOL_TRUE == quoted) ? 1 : 0;
        result++;
    }
    return result;
}
/*--------------------------------------------------------- */
