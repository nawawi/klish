#include "cache.h"

/*--------------------------------------------------------- */
size_t
lub_heap__get_max_free(lub_heap_t *this)
{
    size_t                 result = 0;
    lub_heap_free_block_t *free_block;
    
    /* get the last block in the tree */
    free_block = lub_bintree_findlast(&this->free_tree);
    if(NULL != free_block)
    {
        result = (free_block->tag.words << 2) - sizeof(lub_heap_alloc_block_t);
    }
    return result;
}
/*--------------------------------------------------------- */
