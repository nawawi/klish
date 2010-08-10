/*
 * heap_foreach_free_block.c
 */
#include "private.h"
/*--------------------------------------------------------- */
void
lub_heap_foreach_free_block(lub_heap_t          *this,
                            lub_heap_foreach_fn *fn,
                            void                *arg)
{
    lub_heap_block_t      *block = lub_bintree_findfirst(&this->free_tree);
    lub_bintree_iterator_t iter;
    unsigned int           i = 1;
    
    for(lub_bintree_iterator_init(&iter,&this->free_tree,block);
        block;
        block = lub_bintree_iterator_next(&iter))
    {
        /* call the client function */
        fn(&block->free,
           i++,
           (block->free.tag.words << 2) - sizeof(lub_heap_alloc_block_t),
           arg);        
    }
}
/*--------------------------------------------------------- */
