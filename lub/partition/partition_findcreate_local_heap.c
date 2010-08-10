/*
 * partition_findcreate_local_heap.c
 */
#include "private.h"

/*-------------------------------------------------------- */
lub_heap_t *
lub_partition_findcreate_local_heap(lub_partition_t *this) 
{
    /* see whether one already exists */
    lub_heap_t *local_heap = 0;
    if(this->m_spec.use_local_heap)
    {
        local_heap = lub_partition__get_local_heap(this);
        if(!local_heap)
        {
            size_t required;
        
            /* work out how big the local heap will be... */
            required = lub_heap_overhead_size(this->m_spec.max_local_block_size,
                                              this->m_spec.num_local_max_blocks);
            /* 
             * create a local heap for the current thread which 
             * just contains a cache
             */
            (void)lub_partition_global_realloc(this,
                                               (char**)&local_heap,
                                               required,
                                               LUB_HEAP_ALIGN_NATIVE);
            if(local_heap)
            {
                /* initialise the heap object */
                lub_heap_create(local_heap,required);
                lub_heap_cache_init(local_heap,
                                    this->m_spec.max_local_block_size,
                                    this->m_spec.num_local_max_blocks);
            
                /* store this in the thread specific storage */
                lub_partition__set_local_heap(this,local_heap);
            }
        }
    }
    return local_heap;
}
/*-------------------------------------------------------- */
