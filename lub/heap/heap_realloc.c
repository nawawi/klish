#include <string.h>
#include <assert.h>

#include "cache.h"

/*--------------------------------------------------------- */
lub_heap_status_t
lub_heap_realloc(lub_heap_t      *this,
                 char           **ptr,
                 size_t           requested_size,
                 lub_heap_align_t alignment)
{
    lub_heap_status_t status   = LUB_HEAP_FAILED;
    size_t            size     = requested_size;
    
    /* opportunity for leak detection to do it's stuff */
    lub_heap_pre_realloc(this,ptr,&size);

    if(this->cache && (LUB_HEAP_ALIGN_NATIVE == alignment))
    {
        /* try and get the memory from the cache */
        status = lub_heap_cache_realloc(this,ptr,size);
    }
    else
    {
        /* get the memory directly from the dynamic heap */
        status = lub_heap_raw_realloc(this,ptr,size,alignment);
    }
    
    /* opportunity for leak detection to do it's stuff */
    lub_heap_post_realloc(this,ptr);
    
    if(LUB_HEAP_OK == status)
    {
        /* update the high tide markers */
        if(  (this->stats.alloc_bytes + this->stats.alloc_overhead) 
           > (this->stats.alloc_hightide_bytes + this->stats.alloc_hightide_overhead))
        {
            this->stats.alloc_hightide_blocks   = this->stats.alloc_blocks;
            this->stats.alloc_hightide_bytes    = this->stats.alloc_bytes;
            this->stats.alloc_hightide_overhead = this->stats.alloc_overhead;
            this->stats.free_hightide_blocks    = this->stats.free_blocks;
            this->stats.free_hightide_bytes     = this->stats.free_bytes;
            this->stats.free_hightide_overhead  = this->stats.free_overhead;
        }
    }
    else
    {
        /* this call enables a debugger to catch failures */
        lub_heap_stop_here(status,*ptr,requested_size);
    }
    if((0 == requested_size) && (NULL == *ptr) && (LUB_HEAP_OK == status))
    {
        /* make sure that client doesn't use this (non-existant memory) */
        *ptr = LUB_HEAP_ZERO_ALLOC;
    }
    return status;
}
/*--------------------------------------------------------- */
