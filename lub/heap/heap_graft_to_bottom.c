#include <assert.h>

#include "private.h"

/*--------------------------------------------------------- */
void
lub_heap_graft_to_bottom(lub_heap_t       *this,
                         lub_heap_block_t *free_block,
                         void             *ptr,
                         words_t           words,
                         bool_t            seg_start,
                         bool_t            other_free_block)
{
    lub_heap_tag_t *tail;
    words_t         new_words;
    bool_t          seg_end;

    assert(1 == free_block->free.tag.free);

    /* remove the previous block from the free tree */
    lub_bintree_remove(&this->free_tree,free_block);

    /* update the size of the next block */
    this->stats.free_bytes += (words << 2);

    new_words = words + free_block->free.tag.words;

    tail    = lub_heap_block__get_tail(free_block);
    seg_end = tail->segment;

    if(BOOL_FALSE == other_free_block)
    {
        /* 
         * taint the memory being given back 
         * NB. we don't bother to taint all the memory if we are
         * combining two free blocks; they'll already be tainted.
         */
        lub_heap_taint_memory((char*)ptr,LUB_HEAP_TAINT_FREE,(words << 2)+sizeof(lub_heap_free_block_t));
    }
    else
    {
        /* clear the free block details */
        lub_heap_taint_memory((char*)free_block,LUB_HEAP_TAINT_FREE,sizeof(lub_heap_free_block_t));
    }

    free_block = ptr;
    /* make sure we retain the segment details */
    free_block->free.tag.free    = 1;
    free_block->free.tag.words   = new_words;
    free_block->free.tag.segment = seg_start;
    lub_bintree_node_init(&free_block->free.bt_node);

    /* and update the tail */
    tail->free    = 1;
    tail->words   = free_block->free.tag.words;
    tail->segment = seg_end;

    /* insert back into the tree */
    lub_bintree_insert(&this->free_tree,free_block);
}
/*--------------------------------------------------------- */
