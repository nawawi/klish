#include "../private.h"
#include "../node.h"

#ifdef __CYGWIN__
#  define START _bss_start__
#  define END   _bss_end__
#else
#  define START edata
#  define END   end
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
