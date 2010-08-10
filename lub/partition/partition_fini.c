/*
 * partition_fini.c
 */
#include <stdlib.h>

#include "private.h"

/*-------------------------------------------------------- */
void
lub_partition_fini(lub_partition_t *this)
{
    lub_heap_t *local_heap = lub_partition__get_local_heap(this);
    
    if(local_heap)
    {
        lub_partition_destroy_local_heap(this,local_heap);
    }
    /*
     * we need to have destroyed any client threads 
     * before calling this function
     */
    lub_partition_lock(this);
    lub_heap_destroy(this->m_global_heap);
    lub_partition_unlock(this);
}
/*-------------------------------------------------------- */
