#include "private.h"

/*--------------------------------------------------------- */
lub_heap_block_t *
lub_heap_block_getfirst(const lub_heap_segment_t *seg)
{
    return (lub_heap_block_t*)&seg[1];
}
/*--------------------------------------------------------- */
