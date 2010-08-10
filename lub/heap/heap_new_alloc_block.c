#include "private.h"

/*--------------------------------------------------------- */
void *
lub_heap_new_alloc_block(lub_heap_t *this,
                         words_t     words)
{
    void                  *result = NULL;
    lub_heap_free_block_t *free_block;
    lub_heap_block_t      *new_block;
    lub_heap_key_t         key;
        
    /* initialise the seach key */
    key.words = words;
    key.block = 0;

    /* find the smallest free block which can take this request */
    free_block = lub_bintree_findnext(&this->free_tree,&key);
    if(NULL != free_block)
    {
        lub_heap_tag_t *tail;
        unsigned int    seg_end, seg_start;

        /* remember if this is the end of a segment */
        tail      = lub_heap_block__get_tail((lub_heap_block_t *)free_block);
        seg_start = free_block->tag.segment;
        seg_end   = tail->segment;

        /* remove the block from the free tree */
        lub_bintree_remove(&this->free_tree,free_block);

        /* now slice the bottom off for the client */
        new_block = lub_heap_slice_from_bottom(this,&free_block,&words,BOOL_FALSE);

        if(NULL != free_block)
        {
            /* put the block back into the tree */
            lub_bintree_insert(&this->free_tree,free_block);
        }
        if(NULL != new_block)
        {
            /* set up the tag details */
            new_block->alloc.tag.segment = seg_start;
            new_block->alloc.tag.free    = 0;
            new_block->alloc.tag.words   = words;

            tail = lub_heap_block__get_tail(new_block);
            if(NULL == free_block)
            {
                /* we've swallowed the whole free block */
                tail->segment = seg_end;
            }
            else
            {
                tail->segment = 0;
            }
            tail->free    = 0;
            tail->words   = words;

            /* update the stats */
            ++this->stats.alloc_blocks;
            this->stats.alloc_bytes       += (words << 2);
            this->stats.alloc_bytes       -= sizeof(lub_heap_alloc_block_t);
            ++this->stats.alloc_total_blocks;
            this->stats.alloc_total_bytes += (words << 2);
            this->stats.alloc_total_bytes -= sizeof(lub_heap_alloc_block_t);
            this->stats.alloc_overhead    += sizeof(lub_heap_alloc_block_t);

            /* fill out the client's pointer */
            result = new_block->alloc.memory;
        }
    }
    return result;
}
/*--------------------------------------------------------- */
