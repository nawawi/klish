#include "../private.h"
#include "../node.h"

#ifdef TARGET_SIMNT
#  define START _bss_start__
#  define END   _bss_end__
#else
#  define START wrs_kernel_bss_start
#  define END   wrs_kernel_bss_end
#endif

extern char START, END;

/*--------------------------------------------------------- */
void
lub_heap_scan_bss(void)
{
    /* now scan the memory */
    lub_heap_scan_memory(&START,&END-&START);
}
/*--------------------------------------------------------- */
