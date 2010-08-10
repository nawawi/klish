#include <assert.h>
#include "private.h"

/*--------------------------------------------------------- */
bool_t
lub_heap_extend_both_ways(lub_heap_t        *this,
                          lub_heap_block_t **ptr_block,
                          const words_t      words)
{
    lub_heap_block_t      *block      = *ptr_block;
    lub_heap_free_block_t *prev_block = (lub_heap_free_block_t*)lub_heap_block_getprevious(block);
    lub_heap_free_block_t *next_block = (lub_heap_free_block_t*)lub_heap_block_getnext(block);
    bool_t                 result     = BOOL_FALSE;

    if(prev_block && prev_block->tag.free && next_block && next_block->tag.free)
    {
        /* OK we have a free block above and below us */
        if(words <= (prev_block->tag.words + next_block->tag.words))
        {
            /* There is sufficient space to extend */
            words_t delta = words - next_block->tag.words;
            /* maximise the upward extension */
            result = lub_heap_extend_downwards(this,&block,delta);
            assert(BOOL_TRUE == result);
            result = lub_heap_extend_upwards(this,&block,next_block->tag.words);
            assert(BOOL_TRUE == result);
        }
    }
    return result;
}
/*--------------------------------------------------------- */
