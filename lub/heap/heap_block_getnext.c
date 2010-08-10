#include "private.h"

/*--------------------------------------------------------- */
lub_heap_block_t *
lub_heap_block_getnext(lub_heap_block_t *this)
{
    lub_heap_block_t *result = NULL;
    lub_heap_tag_t   *tail   = lub_heap_block__get_tail(this);

    /* only go forward if this is not the last block in the segment */
    if(0 == tail->segment)
    {
        /* get a pointer to the first word in the next block */
        result = (lub_heap_block_t *)&tail[1]; 
    }
    return result;
}
/*--------------------------------------------------------- */
