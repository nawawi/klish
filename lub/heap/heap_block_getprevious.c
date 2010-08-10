#include "private.h"

/*--------------------------------------------------------- */
lub_heap_block_t *
lub_heap_block_getprevious(lub_heap_block_t *this)
{
    lub_heap_block_t *result = NULL;

    /* only go back if this is not the first block in the segment */
    if(0 == this->alloc.tag.segment)
    {
        /* get the tag from the previous block */
        lub_heap_tag_t *tag = &(&this->alloc.tag)[-1];
        
        /* get a pointer to the previous block 
         * +1 required to account for header tag
         */
        tag = &tag[1-tag->words];
        
        /* now point to the start of the previous block */
        result = (lub_heap_block_t *)tag;
    }
    return result;
}
/*--------------------------------------------------------- */
