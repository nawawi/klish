/*
 * Generic stacktrace function
 */
#ifdef __GNUC__
#include <string.h>

#include "private.h"
#include "context.h"

/*--------------------------------------------------------- */
void
lub_heap__get_stackframe(stackframe_t *stack,
                         unsigned      max)
{
#define GET_RETURN_ADDRESS(n)                                                       \
    {                                                                               \
        if(n < max)                                                                 \
        {                                                                           \
            unsigned long fn = (unsigned long)__builtin_return_address(n+2);        \
            if(!fn || (fn > (unsigned long)lub_heap_data_end))                      \
            {                                                                       \
                break;                                                              \
            }                                                                       \
            stack->backtrace[n] = (function_t*)fn;                                  \
        }                                                                           \
        else                                                                        \
        {                                                                           \
            break;                                                                  \
        }                                                                           \
    }                                                                               \

    memset(stack,0,sizeof(*stack));
    do
    {
        GET_RETURN_ADDRESS(0);
        GET_RETURN_ADDRESS(1);
        GET_RETURN_ADDRESS(2);
        GET_RETURN_ADDRESS(3);
        GET_RETURN_ADDRESS(4);
        GET_RETURN_ADDRESS(5);
        GET_RETURN_ADDRESS(6);
        GET_RETURN_ADDRESS(7);
        GET_RETURN_ADDRESS(8);
        GET_RETURN_ADDRESS(9);
        GET_RETURN_ADDRESS(10);
        GET_RETURN_ADDRESS(11);
        GET_RETURN_ADDRESS(12);
        GET_RETURN_ADDRESS(13);
        GET_RETURN_ADDRESS(14);
        GET_RETURN_ADDRESS(15);
    } while(0);
}
/*--------------------------------------------------------- */
#else /* not __GNUC__ */
    #warning "Generic stack backtrace not implemented for non-GCC compiler..."
#endif /* not __GNUC__*/
