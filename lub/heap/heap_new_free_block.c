#include <assert.h>

#include "private.h"

/*--------------------------------------------------------- */
lub_heap_status_t 
lub_heap_new_free_block(lub_heap_t       *this,
                        lub_heap_block_t *block)
{
    lub_heap_status_t result = LUB_HEAP_OK;
    bool_t            seg_start, seg_end;
    lub_heap_tag_t   *tail;
    
    assert(0 == block->alloc.tag.free);
    
    /* get the segment details from the current block */
    seg_start = block->alloc.tag.segment;
    tail = lub_heap_block__get_tail(block);
    seg_end = tail->segment;

    /* now set up the free block */
    lub_heap_init_free_block(this,
                             block,
                             (tail->words << 2),
                             seg_start,
                             seg_end);
    
    return result;
}
/*--------------------------------------------------------- */
