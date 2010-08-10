#include "string.h"

#include "private.h"
#include "context.h"

/* flag to indicate whether to taint memory or not */
static bool_t tainted = BOOL_FALSE;

/*--------------------------------------------------------- */
bool_t
lub_heap_taint(bool_t enable)
{
    bool_t result = tainted;
    
    tainted = enable;
    
    return result;
}
/*--------------------------------------------------------- */
bool_t
lub_heap_is_tainting(void)
{
    return tainted;
}
/*--------------------------------------------------------- */
void
lub_heap_taint_memory(char            *ptr,
                      lub_heap_taint_t type,
                      size_t           size)
{
    if(BOOL_TRUE == tainted)
    {
#ifdef __vxworks
extern function_t taskDestroy; /* fiddle a reference to the function ...*/
extern function_t taskSuspend; /* ...and the next function in the library */
        /* 
         * VxWorks taskDestroy() relies on being able to access
         * free memory.
         * It calls objFree() followed by objTerminate() !!!!
         * So we have to perform a crufty hack to avoid this
         * mistake blowing us out of the water...
         */
        if(LUB_HEAP_TAINT_FREE == type)
        {
            int i;
            /* obtain the backtrace of the stack */
            stackframe_t frame;
            lub_heap__get_stackframe(&frame,10);
            for(i = 0; i < 10; i++)
            {
                function_t *address = frame.backtrace[i];
                if((address > &taskDestroy) && (address < &taskSuspend))
                {
                    return;
                }
            }
        }
#endif /* __vxworks */
        /* taint the memory */
        memset(ptr,type,size);
    }
}
/*--------------------------------------------------------- */
