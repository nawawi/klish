/*
 * partition_check_memory.c
 */
#include "private.h"

/*-------------------------------------------------------- */
bool_t 
lub_partition_check_memory(lub_partition_t *this)
{
    bool_t      result     = BOOL_TRUE;
    lub_heap_t *local_heap = lub_partition__get_local_heap(this);
    if(local_heap)
    {
        result = lub_heap_check_memory(local_heap);
    }
    if(BOOL_TRUE == result)
    {
        lub_partition_lock(this);
        result = lub_heap_check_memory(this->m_global_heap);
        lub_partition_unlock(this);
    }
    return result;
}
/*-------------------------------------------------------- */
