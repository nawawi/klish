/*
 * partition_destroy_local_heap.c
 */
#include <stdlib.h>
 
#include "private.h"

/*-------------------------------------------------------- */
void
lub_partition_destroy_local_heap(lub_partition_t *this,
                                 lub_heap_t      *local_heap) 
{
    lub_heap_destroy(local_heap);
    /* now release the memory */
    lub_partition_global_realloc(this,(char**)&local_heap,0,LUB_HEAP_ALIGN_NATIVE);
}
/*-------------------------------------------------------- */
