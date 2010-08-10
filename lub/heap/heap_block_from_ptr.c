#include "private.h"

/*--------------------------------------------------------- */
lub_heap_block_t *
lub_heap_block_from_ptr(char *ptr)
{
    return ((lub_heap_block_t*)&((lub_heap_tag_t*)ptr)[-1]);
}
/*--------------------------------------------------------- */
