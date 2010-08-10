#include "private.h"

/*--------------------------------------------------------- */
lub_heap_tag_t *
lub_heap_block__get_tail(lub_heap_block_t *this)
{
    lub_heap_tag_t *ptr = &this->alloc.tag;
    return (ptr + this->alloc.tag.words - 1);
}
/*--------------------------------------------------------- */
