/*
 * argv_delete.c
 */
#include "private.h"
#include "lub/string.h"

#include <stdlib.h>
/*--------------------------------------------------------- */
static void
lub_argv_fini(lub_argv_t *this)
{
    unsigned i;

    for(i = 0;
        i < this->argc;
        i++)
    {
        lub_string_free(this->argv[i].arg);
    }
    free(this->argv);
    this->argv = NULL;
}
/*--------------------------------------------------------- */
void
lub_argv_delete(lub_argv_t *this)
{
    lub_argv_fini(this);
    free(this);
}
/*--------------------------------------------------------- */
