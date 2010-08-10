/*
 * partition_realloc.c
 */
#include <string.h>

#include "private.h"

/*--------------------------------------------------------- */
void
lub_partition_stop_here(lub_heap_status_t status,
                        char             *old_ptr,
                        size_t            new_size)
{
    /* This is provided as a debug aid */
    status   = status;
    old_ptr  = old_ptr;
    new_size = new_size;
}
/*--------------------------------------------------------- */
void
lub_partition_time_to_die(lub_partition_t *this)
{
    bool_t           time_to_die = BOOL_TRUE;
    lub_heap_stats_t stats;
    lub_heap_t      *local_heap = lub_partition__get_local_heap(this);
    if(local_heap)
    {
        lub_heap__get_stats(local_heap,&stats);
        if(stats.alloc_blocks)
        {
            time_to_die = BOOL_FALSE;
        }
    }
    if(BOOL_FALSE == time_to_die)
    {
        lub_partition_lock(this);
        lub_heap__get_stats(this->m_global_heap,&stats);
        lub_partition_unlock(this);
        if(stats.alloc_blocks)
        {
            time_to_die = BOOL_FALSE;
        }
    }
    if(BOOL_TRUE == time_to_die)
    {
        /* and so the time has come... */
        lub_partition_destroy(this);
    }
}
/*--------------------------------------------------------- */
lub_heap_status_t
lub_partition_global_realloc(lub_partition_t *this,
                             char           **ptr,
                             size_t           size,
                             lub_heap_align_t alignment)
{
    lub_heap_status_t status = LUB_HEAP_FAILED;
    lub_partition_lock(this);
    if(!this->m_global_heap)
    {
        /*
         * Create the global heap
         */
        size_t required     = size;
        void  *segment      = lub_partition_segment_alloc(this,&required);
        this->m_global_heap = lub_heap_create(segment,required);
    }
    if(this->m_global_heap)
    {
        status = lub_heap_realloc(this->m_global_heap,ptr,size,alignment);
        if(LUB_HEAP_FAILED == status)
        {
            /* expand memory and try again */
            if(lub_partition_extend_memory(this,size))
            {
                status = lub_heap_realloc(this->m_global_heap,
                                          ptr,size,alignment);
            }
        }
    }
    lub_partition_unlock(this);
    return status;
}
/*--------------------------------------------------------- */
lub_heap_status_t
lub_partition_realloc(lub_partition_t *this,
                      char           **ptr,
                      size_t           size,
                      lub_heap_align_t alignment)
{
    lub_heap_status_t status         = LUB_HEAP_FAILED;
    size_t            requested_size = size;
    lub_heap_t       *local_heap;
    
    if((0 == size) && (0 == *ptr))
    {
        /* simple optimisation */
        return LUB_HEAP_OK;
    }
    if(this->m_dying)
    {
        local_heap = lub_partition__get_local_heap(this);
        if(size)
        {
            /* don't allocate any new memory when dying */
            size = 0;
        }
    }
    else
    {
        local_heap = lub_partition_findcreate_local_heap(this);
    }
    if(local_heap)
    {
        /* try the fast local heap first */
        status = lub_heap_realloc(local_heap,ptr,size,alignment);
    }
    if((LUB_HEAP_FAILED == status) || (LUB_HEAP_INVALID_POINTER == status))
    {
        char *old_ptr = 0;
        
        if(local_heap && (LUB_HEAP_FAILED == status) && size)
        {
            /* we need to reallocate from the global and copy from the local */
            old_ptr = *ptr;
            *ptr    = 0;
        }
        /* time to use the slower global heap */
        status = lub_partition_global_realloc(this,ptr,size,alignment);
        if(old_ptr && (LUB_HEAP_OK == status))
        {
            /* copy from the local to the global */
            memcpy(*ptr,old_ptr,size);

            /* and release the local block */
            status = lub_heap_realloc(local_heap,&old_ptr,0,alignment);
        }
    }
    if(LUB_HEAP_OK != status)
    {
        lub_partition_stop_here(status,*ptr,size);
    }
    /* allow the partion to destroy itself if necessary */
    if(this->m_dying)
    {
        lub_partition_time_to_die(this);
        if(requested_size)
        {
            /* we don't allocate memory whilst dying */
            status = LUB_HEAP_FAILED;
        }
    }
    return status;
}
/*-------------------------------------------------------- */
