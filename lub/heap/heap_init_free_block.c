#include "private.h"

/*--------------------------------------------------------- */
void
lub_heap_init_free_block(lub_heap_t *this,
                         void       *start,
                         size_t      size,
                         bool_t      seg_start,
                         bool_t      seg_end)
{
    lub_heap_free_block_t *block = start;
    lub_heap_tag_t        *tail;
    words_t                words;
    
    /* initialise the given memory */
    lub_heap_taint_memory(start,LUB_HEAP_TAINT_FREE,size);

    /* calculate the number of words in this segment */
    words  = (size >> 2);
    
    /* setup the block */
    block->tag.segment = seg_start; /* start of a segment */
    block->tag.free    = 1;
    block->tag.words   = words;
    
    /* initialise the tree node */
    lub_bintree_node_init(&block->bt_node);
    
    /* now fill out the trailing tag */
    tail          = lub_heap_block__get_tail((lub_heap_block_t*)block);
    tail->segment = seg_end; /* end of a segment */
    tail->free    = 1;
    tail->words   = words;

    /* now insert this free block into the tree */
    lub_bintree_insert(&this->free_tree,block);

    ++this->stats.free_blocks;
    this->stats.free_bytes    += (words << 2);
    this->stats.free_bytes    -= sizeof(lub_heap_alloc_block_t);
    this->stats.free_overhead += sizeof(lub_heap_alloc_block_t);
}
/*--------------------------------------------------------- */
