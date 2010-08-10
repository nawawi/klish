#include <assert.h>

#include "private.h"

/*--------------------------------------------------------- */
void
lub_heap_graft_to_top(lub_heap_t       *this,
                      lub_heap_block_t *free_block,
                      void             *ptr,
                      words_t           words,
                      bool_t            seg_end,
                      bool_t            other_free_block)
{
    lub_heap_tag_t   *tail;
    
    assert(1 == free_block->free.tag.free);

    /* remove the previous block from the free tree */
    lub_bintree_remove(&this->free_tree,free_block);

    if(BOOL_FALSE == other_free_block)
    {
        tail = lub_heap_block__get_tail(free_block);
        /* 
         * taint the memory being given back 
         * NB. we don't bother to taint all the memory if we are
         * combining two free blocks; they'll already be tainted.
         */
        lub_heap_taint_memory((char*)tail,
                              LUB_HEAP_TAINT_FREE,
                              (words << 2) + sizeof(lub_heap_tag_t));
    }
    else
    {
        char *tmp = ptr;
        /* clear out the freeblock details */
        lub_heap_taint_memory(tmp - sizeof(lub_heap_tag_t),
                              LUB_HEAP_TAINT_FREE,
                              sizeof(lub_heap_tag_t) + sizeof(lub_heap_free_block_t));
    }
    /* update the size of the previous block */
    free_block->free.tag.words  += words;
    this->stats.free_bytes      += (words << 2);

    /* make sure we retain the segment details */
    tail          = lub_heap_block__get_tail(free_block);
    tail->segment = seg_end;
    tail->free    = 1;

    /* update the size of the tail */
    tail->words = free_block->free.tag.words;

    /* insert back into the tree */
    lub_bintree_insert(&this->free_tree,free_block);
}
/*--------------------------------------------------------- */
