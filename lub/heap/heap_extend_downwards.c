#include "private.h"

/*--------------------------------------------------------- */
bool_t
lub_heap_extend_downwards(lub_heap_t        *this,
                          lub_heap_block_t **ptr_block,
                          const words_t      words)
{
    lub_heap_block_t      *block      = *ptr_block;
    lub_heap_free_block_t *prev_block = (lub_heap_free_block_t*)lub_heap_block_getprevious(block);
    bool_t                 result     = BOOL_FALSE;
    
    if(NULL != prev_block)
    {
        /* Do we have a free block below us? */
        if(1 == prev_block->tag.free)
        {
            int   segment;
            void *tmp;
            
            /* work out how many extra words we need */
            words_t delta = words - block->alloc.tag.words;
    
            /* remove the block from the tree */
            lub_bintree_remove(&this->free_tree,prev_block);
            
            /* remember the segment status */
            segment = prev_block->tag.segment;
            
            /* try and slice off a chunk of memory */
            tmp = lub_heap_slice_from_top(this,&prev_block,&delta,BOOL_FALSE);

            if(NULL != prev_block)
            {
                /* put the modified free block back into the tree */
                lub_bintree_insert(&this->free_tree,prev_block);
                
                /* there is still a block below us */
                segment = 0;
            }
            if(NULL != tmp)
            {
                /* we managed to extend downwards */
                lub_heap_tag_t   *tail      = lub_heap_block__get_tail(block);
                lub_heap_block_t *new_block = tmp;

                /* get ready to copy the data down accounts for the header */
                words_t              old_words = tail->words;
                const unsigned int  *src       = (unsigned int*)block;
                unsigned int        *dst       = (unsigned int*)new_block;

                /* copy the old data to the new location */
                while(old_words--)
                {
                    *dst++ = *src++;
                }
                /* taint the extra memory */
                lub_heap_taint_memory(((char*)tail) - (delta<<2),
                                      LUB_HEAP_TAINT_ALLOC,
                                      (delta<<2));
                
                /* fill out the new block details */
                tail->words += delta;
                this->stats.alloc_bytes       += (delta << 2);
                this->stats.alloc_total_bytes += (delta << 2);

                new_block->alloc.tag.segment = segment;
                new_block->alloc.tag.free    = 0;
                new_block->alloc.tag.words   = tail->words;
                
                /* update the clients pointer */
                *ptr_block = new_block;

                result = BOOL_TRUE;
            }
        }
    }
    return result;
}
/*--------------------------------------------------------- */
