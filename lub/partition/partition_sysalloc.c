/*
 * partition_sysalloc.c
 */
#include <stdlib.h>

#include "private.h"

/*-------------------------------------------------------- */
void *
lub_partition_sysalloc(lub_partition_t *this,
                       size_t           required)
{
    void *result = 0;
    if(required < this->m_partition_ceiling)
    {
        result = this->m_spec.sysalloc(required);
        if(result)
        {
            this->m_partition_ceiling -= required;
        }
    }
    return result;
}
/*-------------------------------------------------------- */
