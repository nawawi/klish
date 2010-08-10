/*
 * argv__get_argv.c
 */

#include <stdlib.h>

#include "private.h"

/*--------------------------------------------------------- */
char **
lub_argv__get_argv(const lub_argv_t *this, char *argv0)
{
    char **result = NULL;
    unsigned i;
    unsigned a = 0;

    if(argv0)
        a = 1;

    result = malloc(sizeof(char *) * (this->argc + 1 + a));

    if(argv0)
        result[0] = argv0;
    for(i = 0; i < this->argc; i++)
        result[i + a] = this->argv[i].arg;
    result[i + a] = NULL;

    return result;
}
/*--------------------------------------------------------- */
