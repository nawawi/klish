#include "../private.h"
#include "../node.h"

#ifdef __CYGWIN__
#  define START _data_start__
#  define END   _data_end__
#else
#  define START etext
#  define END   edata
#endif

extern char START, END;
char *lub_heap_data_start = &START;
 char *lub_heap_data_end   = &END;

/*--------------------------------------------------------- */
void
lub_heap_scan_data(void)
{
    /* now scan the memory */
    lub_heap_scan_memory(lub_heap_data_start,
                         lub_heap_data_end-lub_heap_data_start);
}
/*--------------------------------------------------------- */
