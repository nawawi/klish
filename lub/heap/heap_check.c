#include "private.h"

/* flag to indicate whether to check memory or not */
static bool_t checking = BOOL_FALSE;

typedef struct 
{
    bool_t      result;
    unsigned    free_blocks;
    lub_heap_t *heap;
} check_arg_t;
/*--------------------------------------------------------- */
static void
process_segment(void    *s,
                unsigned index,
                size_t   size,
                void    *a)
{
    lub_heap_segment_t *segment = s;
    lub_heap_block_t   *block   = lub_heap_block_getfirst(segment);
    check_arg_t        *arg     = a;
    /*
     * don't bother checking if this segment has zero blocks
     */
    if(arg->heap->stats.free_blocks || arg->heap->stats.alloc_blocks)
    {
        arg->result = lub_heap_block_check(block);
        if(BOOL_TRUE == arg->result)
        {
            /* check that the first block has the segment set */
            arg->result = (block->free.tag.segment == 1) ? BOOL_TRUE : BOOL_FALSE;
            if(block->free.tag.free)
            {
                ++arg->free_blocks;
            }
        }
        
        /* now iterate along the block chain checking each one */
        while((BOOL_TRUE == arg->result) 
              && (block = lub_heap_block_getnext(block)))
        {
            /* check this block */
            arg->result = lub_heap_block_check(block);
            if(BOOL_TRUE == arg->result)
            {
                /* ensure that all non-first blocks have the segment clear */
                arg->result = (block->free.tag.segment == 0) ? BOOL_TRUE : BOOL_FALSE;
                if(block->free.tag.free)
                {
                    ++arg->free_blocks;
                }
            }
        }
    }
}

/*--------------------------------------------------------- */
bool_t
lub_heap_check(bool_t enable)
{
    bool_t result = checking;
    
    checking = enable;
    
    return result;
}
/*--------------------------------------------------------- */
bool_t
lub_heap_is_checking(void)
{
    return checking;
}
/*--------------------------------------------------------- */
bool_t
lub_heap_check_memory(lub_heap_t *this)
{
    check_arg_t arg;
    
    arg.result = BOOL_TRUE;
    
    if(BOOL_TRUE == checking)
    {
        arg.free_blocks = 0;
        arg.heap        = this;
        /* iterate all the segments in the system */
        lub_heap_foreach_segment(this,process_segment,&arg);
    
        /* now check that the stats map to the available free blocks 
         * checking the alloc blocks is more difficult because of the hidden
         * overheads when performing leak detection.
         */
        if( (this->stats.free_blocks != arg.free_blocks) )
        {
            arg.result = BOOL_FALSE;
        }
    }
    return arg.result;
}
/*--------------------------------------------------------- */
