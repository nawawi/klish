#include <assert.h>

#include "private.h"
/*--------------------------------------------------------- */
void *
lub_heap_slice_from_bottom(lub_heap_t             *this,
                           lub_heap_free_block_t **ptr_block,
                           words_t                *words,
                           bool_t                  seg_start)
{
    void                  *result = NULL;
    lub_heap_free_block_t *block  = *ptr_block;

    /* check this is a free block */
    assert(1 == block->tag.free);
    
    if(BOOL_TRUE == lub_heap_block_check((lub_heap_block_t*)block)
        && (*words <= block->tag.words))
    {
        words_t new_words = (block->tag.words - *words);
        
        /* get the tag at the end of the current free block */
        lub_heap_tag_t *tail = lub_heap_block__get_tail((lub_heap_block_t*)block);

        /* 
         * Is there sufficient memory to perform the task? 
         * we must leave enough space for the free block to work
         */
        if(new_words > (sizeof(lub_heap_free_block_t) >> 2))
        {
            lub_heap_free_block_t *new_block;
        
            /* adjust the free block size */
            tail->words -= *words;
        
            /* create a new free block header */
            new_block = (lub_heap_free_block_t *)(&block->tag + *words);
            new_block->tag.segment = seg_start;
            new_block->tag.free    = 1; /* this is a free block */
            new_block->tag.words   = tail->words;

            /* update the stats */
            this->stats.free_bytes -= (*words << 2);

            /* set up the tree node */
            lub_bintree_node_init(&new_block->bt_node);
        
            result     = block;
            *ptr_block = new_block;
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
