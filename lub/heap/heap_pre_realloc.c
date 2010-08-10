#include <string.h>

#include "private.h"
#include "context.h"
#include "node.h"

/*
 * This function
 */
void
lub_heap_pre_realloc(lub_heap_t *this,
                     char      **ptr,
                     size_t     *size)
{
    if(*ptr)
    {
        /* is this a pointer to a node in the "leak" trees? */
        lub_heap_node_t *node = lub_heap_node_from_start_of_block_ptr(*ptr);
        if(NULL != node)
        {
            lub_heap_node_fini(node);
             /* move the pointer to the start of the block */
            *ptr = (char*)node;
        }
    }
    if((0 < lub_heap_frame_count))
    {
        size_t old_size = *size;
        if(old_size)
        {
            /* allocate enough bytes for a node */
            *size += lub_heap_node__get_instanceSize();
            if(*size < old_size)
            {
                /* we've wrapped the size variable
                 * make sure we fail the allocation
                 */
                *size = (size_t)-1;
            }
        }
    }
}
