#include <stdlib.h>

#include "private.h"

/*--------------------------------------------------------- */
void *
lub_dblockpool_alloc(lub_dblockpool_t *this)
{
    void                    *result = NULL;
    lub_dblockpool_chunk_t *chunk;
    unsigned                chunk_count = 0;
    
    /* first find a chunk which can service this request */
    for(chunk = this->first_chunk;
        chunk;
        chunk = chunk->next)
    {
        chunk_count++;
        /* try and get a block from this chunk */
        result = lub_blockpool_alloc(&chunk->pool);
        if(NULL != result)
        {
            /* got some memory */
            break;
        }
    }
    if( (NULL == result) 
        && (!this->max_chunks || (chunk_count < this->max_chunks)) )
    {
        /* dynamically allocate a new chunk */
        chunk = malloc(sizeof(lub_dblockpool_chunk_t) + (this->block_size * this->chunk_size));
        if(NULL != chunk)
        {
            /* configure the new chunk */
            chunk->next = this->first_chunk;
            lub_blockpool_init(&chunk->pool,
                               &chunk[1],
                               this->block_size,
                               this->chunk_size);
            this->first_chunk = chunk;
            chunk->count      = 0;
            
            /* now allocate the memory */
            result = lub_blockpool_alloc(&chunk->pool);
        }
    }
    if((NULL != result) && (NULL != chunk))
    {
        /* track the number of allocations */
        chunk->count++;
    }
    return result;
}
/*--------------------------------------------------------- */
