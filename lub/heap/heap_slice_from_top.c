#include <assert.h>

#include "private.h"

/*--------------------------------------------------------- */
void *
lub_heap_slice_from_top(lub_heap_t             *this,
                        lub_heap_free_block_t **ptr_block,
                        words_t                *words,
                        bool_t                  seg_end)
{
    void                  *result = NULL;
    lub_heap_free_block_t *block  = *ptr_block;
    
    /* check this is a free block */
    assert(1 == block->tag.free);

    if(BOOL_TRUE == lub_heap_block_check((lub_heap_block_t*)block)
       && (*words <= block->tag.words))
    {
        words_t         new_words = (block->tag.words - *words);
        lub_heap_tag_t *tail;
    
        /* Is there sufficient memory to perform the task? */
        if(new_words > (sizeof(lub_heap_free_block_t) >> 2))
        {
            /* update the free block */
            block->tag.words -= *words;
        
            /* get the new tail tag */
            tail = lub_heap_block__get_tail((lub_heap_block_t*)block);
        
            /* set up the tag */
            tail->segment = seg_end;
            tail->free    = 1;
            tail->words   = block->tag.words;

            /* update the stats */
            this->stats.free_bytes -= (*words << 2);
        
            result = &tail[1];
        }
        else
        {
            /* 
             * there is only just enough space for the request
             * so we also throw in the memory used for the tags
             */
            --this->stats.free_blocks;
            this->stats.free_bytes    -= (block->tag.words << 2);
            this->stats.free_bytes    += sizeof(lub_heap_alloc_block_t);
            this->stats.free_overhead -= sizeof(lub_heap_alloc_block_t);
            
            /* update the client's word count */
            *words = block->tag.words; 

            /* there is no free block left!!! */
            result     = block;
            *ptr_block = NULL;
        }
        if(NULL != result)
        {
            /* taint the memory */
            lub_heap_taint_memory(result,LUB_HEAP_TAINT_ALLOC,(*words << 2));
        }
    }
    return result;
}
/*--------------------------------------------------------- */
