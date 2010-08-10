#include "private.h"
/*--------------------------------------------------------- */
bool_t
lub_heap_extend_upwards(lub_heap_t        *this,
                        lub_heap_block_t **ptr_block,
                        words_t            words)
{
    lub_heap_block_t      *block      = *ptr_block;
    lub_heap_free_block_t *next_block = (lub_heap_free_block_t *)lub_heap_block_getnext(block);
    bool_t                 result     = BOOL_FALSE;
    
    if(NULL != next_block)
    {
        /* Do we have a free block above us? */
        if(1 == next_block->tag.free)
        {
            int             segment;
            void           *tmp;
            lub_heap_tag_t *tail = lub_heap_block__get_tail((lub_heap_block_t *)next_block);

            /* work out how many extra words we need */
            words -= block->alloc.tag.words;
            
            /* remove the block from the tree */
            lub_bintree_remove(&this->free_tree,next_block);
            
            /* remember the segment status of the next block */
            segment = tail->segment;
            
            /* try and slice off a chunk of memory */
            tmp = lub_heap_slice_from_bottom(this,&next_block,&words,BOOL_FALSE);

            if(NULL != next_block)
            {
                /* put the modified free block back into the tree */
                lub_bintree_insert(&this->free_tree,next_block);

                /* there is still a block above us */
                segment = 0;
            }
            if(NULL != tmp)
            {
                /* we managed to extend upwards */
                result = BOOL_TRUE;

                /* taint the old tail pointer */
                tail = lub_heap_block__get_tail(block);
                lub_heap_taint_memory((char*)tail,LUB_HEAP_TAINT_ALLOC,sizeof(lub_heap_tag_t));
                
                /* fill out the new block details */
                block->alloc.tag.words        += words;
                this->stats.alloc_bytes       += (words << 2);
                this->stats.alloc_total_bytes += (words << 2);

                tail = lub_heap_block__get_tail(block);
                tail->segment = segment;
                tail->free    = 0;
                tail->words   = block->alloc.tag.words;
            }
        }
    }
    return result;
}
/*--------------------------------------------------------- */
