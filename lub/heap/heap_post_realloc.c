#include <string.h>

#include "private.h"
#include "context.h"
#include "node.h"

void
lub_heap_post_realloc(lub_heap_t        *this,
                      char             **ptr)
{
    /* only act if we are about to re-enable the monitoring */
    if((0 < lub_heap_frame_count) && ( 2 > this->suppress))
    {
        if(NULL != *ptr)
        {
            stackframe_t              frame;
            lub_heap_node_t          *node = (lub_heap_node_t*)*ptr;
            const lub_heap_context_t *context;

            lub_heap__get_stackframe(&frame,lub_heap_frame_count);

            /* make sure we break any recursive behaviour */
            ++this->suppress;
            context = lub_heap_context_find_or_create(this,&frame);
            --this->suppress;
            
            /* initialise the node instance */
            lub_heap_node_init(node,(lub_heap_context_t*)context);

            /* make sure we doctor the pointer given back to the client */
            *ptr = lub_heap_node__get_ptr(node);
        }
    }
}

void
lub_heap_leak_suppress_detection(lub_heap_t *this)
{
    ++this->suppress;
}

void
lub_heap_leak_restore_detection(lub_heap_t *this)
{
    --this->suppress;
}
