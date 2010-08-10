#include "private.h"

/*--------------------------------------------------------- */
bool_t
lub_heap_block_check(lub_heap_block_t *this)
{
    lub_heap_tag_t *tail = lub_heap_block__get_tail(this);

    return (tail->words == this->alloc.tag.words) ? BOOL_TRUE : BOOL_FALSE;
}
/*--------------------------------------------------------- */
