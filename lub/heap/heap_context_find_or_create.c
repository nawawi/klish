#include "private.h"
#include "context.h"

/*--------------------------------------------------------- */
const lub_heap_context_t *
lub_heap_context_find_or_create(lub_heap_t         * this,
                                const stackframe_t * stack)
{
    lub_heap_context_t *context;
    
    /*
     * look to see whether there is an 
     * existing context for this allocation 
     */
    context = lub_heap_context_find(stack);
    if(NULL == context)
    {
        lub_heap_leak_t *leak = lub_heap_leak_instance();
        context = lub_dblockpool_alloc(&leak->m_context_pool);
        lub_heap_leak_release(leak);
        
        if(NULL != context)
        {
            /* initialise the instance */
            lub_heap_context_init(context,this,stack);
        }
    }
    return context;
}
/*--------------------------------------------------------- */
