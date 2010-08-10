#include <stdlib.h>

#include "private.h"

/*--------------------------------------------------------- */
void
lub_dblockpool_fini(lub_dblockpool_t *this)
{
    lub_dblockpool_chunk_t *chunk;
    
    /* release all the memory used for contexts */
    while((chunk = this->first_chunk))
    {
        this->first_chunk = chunk->next;
        free(chunk);
    }
}
/*--------------------------------------------------------- */
