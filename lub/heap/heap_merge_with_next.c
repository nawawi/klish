#include <assert.h>

#include "private.h"

/*--------------------------------------------------------- */
lub_heap_status_t
lub_heap_merge_with_next(lub_heap_t       *this,
                         lub_heap_block_t *block)
{
    lub_heap_status_t result = LUB_HEAP_FAILED;
    lub_heap_block_t *next_block;
        
    assert(0 == block->alloc.tag.free);

    do
    {
        /* see whether there is a free block just after us */
        next_block = lub_heap_block_getnext(block);
        if(   (NULL != next_block               ) 
           && (1    == next_block->free.tag.free))
        {
            if(BOOL_FALSE == lub_heap_block_check(next_block))
            {
                result = LUB_HEAP_CORRUPTED;
                break;
            }
            lub_heap_graft_to_bottom(this,
                                     next_block,
                                     block,
                                     block->alloc.tag.words,
                                     block->alloc.tag.segment,
                                     block->free.tag.free ? BOOL_TRUE : BOOL_FALSE);
            result = LUB_HEAP_OK;
        }
    } while(0);
    
    return result;
}
/*--------------------------------------------------------- */
