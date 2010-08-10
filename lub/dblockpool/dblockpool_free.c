#include <stdlib.h>

#include "private.h"

/*--------------------------------------------------------- */
void
lub_dblockpool_free(lub_dblockpool_t *this,
                    void             *block)
{
    lub_dblockpool_chunk_t **chunk_ptr;
    
    /* find the chunk from which this derived. */
    for(chunk_ptr = &this->first_chunk;
        *chunk_ptr;
        chunk_ptr = &(*chunk_ptr)->next)
    {
        const char *pool_start = (char *)&(*chunk_ptr)[1];
        const char *pool_end   = pool_start + (this->block_size * this->chunk_size);
        const char *ptr = block;
        
        if((ptr >= pool_start) && (ptr < pool_end))
        {
            /* found the right pool */
            lub_blockpool_free(&(*chunk_ptr)->pool,block);
            (*chunk_ptr)->count--;
            /* track the number of allocations */
            if(0 == (*chunk_ptr)->count)
            {
                lub_dblockpool_chunk_t *tmp = *chunk_ptr;
                /* removing last block from this chunk */
                *chunk_ptr = (*chunk_ptr)->next;
                free(tmp);
            }
            break;
        }
    }
}
/*--------------------------------------------------------- */
