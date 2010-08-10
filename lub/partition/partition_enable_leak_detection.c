/*
 * partition_enable_leak_detection.c
 */
#include "private.h"
/*-------------------------------------------------------- */
void
lub_partition_enable_leak_detection(lub_partition_t *this)
{
    lub_heap_t *local_heap = lub_partition__get_local_heap(this);
    if(local_heap)
    {
        lub_heap_leak_suppress_detection(local_heap);
    }
    if(this->m_global_heap)
    {
        lub_partition_lock(this);
        lub_heap_leak_suppress_detection(this->m_global_heap);
        lub_partition_unlock(this);
    }
}
/*-------------------------------------------------------- */
