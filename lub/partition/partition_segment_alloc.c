/*
 * partition_segment_alloc.c
 */
#include "private.h"

/*-------------------------------------------------------- */
void *
lub_partition_segment_alloc(lub_partition_t *this,
                            size_t          *required)
{
    if(*required < this->m_spec.min_segment_size)
    {
        *required = (this->m_spec.min_segment_size >> 1);
    }
    /* double the required size */
    *required <<= 1;
    return lub_partition_sysalloc(this,*required);
}
/*-------------------------------------------------------- */
