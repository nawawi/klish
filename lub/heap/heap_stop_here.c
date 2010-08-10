#include <stdio.h>

#include "private.h"
#include "context.h"

/* 
 * This global variable is used to control error output when 
 * lub_heap_stop_here is called
 */
int lub_heap_verbose_errors = 0;

static const char *status_names[] =
{
    "LUB_HEAP_OK",
    "LUB_HEAP_FAILED",
    "LUB_HEAP_DOUBLE_FREE",
    "LUB_HEAP_CORRUPTED",
    "LUB_HEAP_INVALID_POINTER"
};
/*--------------------------------------------------------- */
void
lub_heap_stop_here(lub_heap_status_t status,
                   char             *old_ptr,
                   size_t            new_size)
{
    /* This is provided as a debug aid */
    status   = status;
    old_ptr  = old_ptr;
    new_size = new_size;
    
    if(lub_heap_verbose_errors)
    {
        switch(status)
        {
            case LUB_HEAP_OK:
            case LUB_HEAP_FAILED:
            {
                /* this is normal */
                break;
            }
            case LUB_HEAP_DOUBLE_FREE:
            case LUB_HEAP_CORRUPTED:
            case LUB_HEAP_INVALID_POINTER:
            {
                /* obtain the backtrace of the stack */
                stackframe_t frame;
                long         address;
                int          i;
                lub_heap__get_stackframe(&frame,MAX_BACKTRACE);
                /* and output it */
                printf("lub_heap_stop_here(%s,%p,%"SIZE_FMT")\n",status_names[status],old_ptr,new_size);
                for(i = 0;
                    (address = (long)frame.backtrace[i]);
                    i++)
                {
                    lub_heap_symShow(address);
                    printf("\n");
                }
            }
        }
    }
}
/*--------------------------------------------------------- */
