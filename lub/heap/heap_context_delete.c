#include "private.h"
#include "context.h"

bool_t
lub_heap_context_delete(lub_heap_context_t *context)
{
    lub_heap_leak_t *leak = lub_heap_leak_instance();
    
    /* finalise the instance */
    lub_heap_context_fini(context);

    /* release the memory */
    lub_dblockpool_free(&leak->m_context_pool,context);
    lub_heap_leak_release(leak);
    
    return BOOL_TRUE;
}
