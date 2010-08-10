/*
 * string_dupn.c
 */
#include "private.h"

#include <string.h>
#include <stdlib.h>

/*--------------------------------------------------------- */
char *
lub_string_dupn(const char *string, 
                unsigned    length)
{
    char *result=NULL;
    if(NULL != string)
    {
        result = malloc(length+1);
        if(NULL != result)
        {
            strncpy(result,string,length);
            result[length] = '\0';
        }
    }
    return result;
}
/*--------------------------------------------------------- */
