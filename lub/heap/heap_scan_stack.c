/*
 * Generic stack scanning function
 *
 * This currently only scans the last 32 levels of the current stack.
 */
#ifdef __GNUC__
#include "private.h"
#include "node.h"
/*--------------------------------------------------------- */
void
lub_heap_scan_stack(void)
{
    char *start,*end;
#define FRAME_ADDRESS(n)                            \
    {                                               \
        char *fn = __builtin_return_address(n);     \
        if(!fn || (fn > lub_heap_data_start))       \
        {                                           \
            break;                                  \
        }                                           \
        end = __builtin_frame_address(n);           \
    }                                               \

    start = end = __builtin_frame_address(0);

    do {
        FRAME_ADDRESS(0);
        FRAME_ADDRESS(1);
        FRAME_ADDRESS(2);
        FRAME_ADDRESS(3);
        FRAME_ADDRESS(4);
        FRAME_ADDRESS(5);
        FRAME_ADDRESS(6);
        FRAME_ADDRESS(7);
        FRAME_ADDRESS(8);
        FRAME_ADDRESS(9);
        FRAME_ADDRESS(10);
        FRAME_ADDRESS(11);
        FRAME_ADDRESS(12);
        FRAME_ADDRESS(13);
        FRAME_ADDRESS(14);
        FRAME_ADDRESS(15);
        FRAME_ADDRESS(16);
        FRAME_ADDRESS(17);
        FRAME_ADDRESS(18);
        FRAME_ADDRESS(19);
        FRAME_ADDRESS(20);
        FRAME_ADDRESS(21);
        FRAME_ADDRESS(22);
        FRAME_ADDRESS(23);
        FRAME_ADDRESS(24);
        FRAME_ADDRESS(25);
        FRAME_ADDRESS(26);
        FRAME_ADDRESS(27);
        FRAME_ADDRESS(28);
        FRAME_ADDRESS(29);
        FRAME_ADDRESS(30);
        FRAME_ADDRESS(31);
    } while(0);
    /* now scan the memory */
    lub_heap_scan_memory(start,end-start);
}
/*--------------------------------------------------------- */
#else /* not __GNUC__ */
    #warning "Generic stack scanning not implemented for non-GCC compiler..."
#endif /* not __GNUC__*/
