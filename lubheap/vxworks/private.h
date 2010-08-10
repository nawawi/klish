#include <private/semLibP.h>
#include <private/classLibP.h>

#include "lub/heap.h"

typedef struct partition_s partition_t;
struct partition_s
{
    OBJ_CORE    objCore;   /* object management        */
    SEMAPHORE   sem;       /* partition semaphore      */
    unsigned    options;   /* options                  */
    lub_heap_t *heap;      /* reference to heap object */
};

extern void
    lubheap_vxworks_check_status(partition_t *this,
                                 lub_heap_status_t status,
                                 const char       *where,
                                 void             *block,
                                 size_t            size);
