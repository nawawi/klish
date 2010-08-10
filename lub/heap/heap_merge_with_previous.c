#include "private.h"

/*--------------------------------------------------------- */
lub_heap_status_t
lub_heap_merge_with_previous(lub_heap_t       *this,
                             lub_heap_block_t *block)
{
    lub_heap_status_t result = LUB_HEAP_FAILED;
    lub_heap_block_t *prev_block;

    do
    {
        /* see whether there is a free block just before us */
        prev_block = lub_heap_block_getprevious(block);
        if(   (NULL      != prev_block                   )           
           && (1         == prev_block->free.tag.free    ))
        {
            lub_heap_tag_t *tail;

            if(BOOL_FALSE == lub_heap_block_check(prev_block))
            {
                result = LUB_HEAP_CORRUPTED;
                break;
            }
            tail = lub_heap_block__get_tail(block);

            if(1 == block->free.tag.free)
            {
                /* remove this free block from the tree */
                lub_bintree_remove(&this->free_tree,block);
                --this->stats.free_blocks;
                this->stats.free_bytes    -= (block->free.tag.words << 2);
                this->stats.free_bytes    += sizeof(lub_heap_alloc_block_t);
                this->stats.free_overhead -= sizeof(lub_heap_alloc_block_t);
            }
            /* now add this memory to the previous free block */
            lub_heap_graft_to_top(this,
                                  prev_block,
                                  block,
                                  block->alloc.tag.words,
                                  tail->segment,
                                  block->free.tag.free ? BOOL_TRUE : BOOL_FALSE);
            result = LUB_HEAP_OK;
        }
    } while(0);
    
    return result;
}
/*--------------------------------------------------------- */
