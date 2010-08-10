#include "lub/types.h"
#include "lub/partition.h"
#include "lub/heap.h"
#include "lub/size_fmt.h"


struct _lub_partition
{
    lub_heap_t          *m_global_heap;
    lub_partition_spec_t m_spec;
    bool_t               m_dying;
    size_t               m_partition_ceiling;
};

lub_heap_t *
lub_partition__get_local_heap(lub_partition_t *instance);

void
lub_partition__set_local_heap(lub_partition_t *instance,
                              lub_heap_t      *local_heap);

void 
lub_partition_lock(lub_partition_t *instance);

void
lub_partition_unlock(lub_partition_t *instance);


lub_heap_status_t
lub_partition_global_realloc(lub_partition_t *instance,
                             char           **ptr,
                             size_t           size,
                             lub_heap_align_t alignment);
void *
lub_partition_segment_alloc(lub_partition_t *instance,
                            size_t          *required);

void
lub_partition_destroy_local_heap(lub_partition_t *instance,
                                 lub_heap_t      *local_heap); 
void
lub_partition_time_to_die(lub_partition_t *instance);

bool_t
lub_partition_extend_memory(lub_partition_t *instance,
                             size_t           required);
void
lub_partition_destroy(lub_partition_t *instance);

lub_heap_t *
lub_partition_findcreate_local_heap(lub_partition_t *instance);

void *
lub_partition_sysalloc(lub_partition_t *instance,
                       size_t           required);

void
lub_partition_init(lub_partition_t            *instance,
                   const lub_partition_spec_t *spec);

void
lub_partition_fini(lub_partition_t *instance);
