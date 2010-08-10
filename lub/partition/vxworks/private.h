/*
 * private.h
 */
#include <private/semLibP.h>
#include <taskVarLib.h>

#include "../private.h"

/**********************************************************
 * PRIVATE TYPES
 ********************************************************** */
typedef struct _lub_vxworks_partition lub_vxworks_partition_t;
struct _lub_vxworks_partition
{
    lub_partition_t          m_base;
    lub_heap_t              *m_local_heap;
    SEMAPHORE                m_sem;
    lub_vxworks_partition_t *m_next_partition;
};
