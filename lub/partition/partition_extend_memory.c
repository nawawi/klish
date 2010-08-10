/*
 * partition_extend_memory.c
 */
#include "private.h"

/*-------------------------------------------------------- */
bool_t
lub_partition_extend_memory(lub_partition_t *this,
                             size_t           required)
{
    bool_t result  = BOOL_FALSE;
    void  *segment = lub_partition_segment_alloc(this,&required);
    if(segment)
    {
        lub_partition_lock(this);
        lub_heap_add_segment(this->m_global_heap,segment,required);
        lub_partition_unlock(this);
        result = BOOL_TRUE;
    }
    return result;
}
/*-------------------------------------------------------- */
