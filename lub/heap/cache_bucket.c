#include "cache.h"

/*--------------------------------------------------------- */
size_t
lub_heap_cache_bucket__get_block_overhead(lub_heap_cache_bucket_t *this,
                                          const char              *ptr)
{
    return sizeof(lub_heap_cache_bucket_t*);
}
/*--------------------------------------------------------- */
size_t
lub_heap_cache_bucket__get_block_size(lub_heap_cache_bucket_t *this,
                                      const char              *ptr)
{
    size_t size = 0;

    /* get the size from the cache */
    lub_blockpool_stats_t stats;
    lub_blockpool__get_stats(&this->m_blockpool,&stats);
    size = stats.block_size - sizeof(lub_heap_cache_bucket_t*);

    return size;
}
/*--------------------------------------------------------- */
void
lub_heap_cache_bucket_init(lub_heap_cache_bucket_t *this,
                           lub_heap_cache_t        *cache,
                           size_t                   block_size,
                           size_t                   bucket_size)
{
    /* initialise the blockpool */
    size_t num_blocks = bucket_size/block_size;
    
    lub_blockpool_init(&this->m_blockpool,
                       this->m_memory_start,
                       block_size,
                       num_blocks);
    this->m_memory_end = this->m_memory_start + bucket_size;
    this->m_cache      = cache;
}
/*--------------------------------------------------------- */
void *
lub_heap_cache_bucket_alloc(lub_heap_cache_bucket_t *this)
{
    void                     *ptr        = 0;
    lub_heap_cache_bucket_t **bucket_ptr = lub_blockpool_alloc(&this->m_blockpool);
    if(bucket_ptr)
    {
        *bucket_ptr = this;
        ptr = ++bucket_ptr;
        /* make sure that released memory is tainted */
        lub_heap_taint_memory(ptr,
                              LUB_HEAP_TAINT_ALLOC,
                              this->m_blockpool.m_block_size-sizeof(lub_heap_cache_bucket_t*));
    }
    return ptr;
}
/*--------------------------------------------------------- */
lub_heap_status_t
lub_heap_cache_bucket_free(lub_heap_cache_bucket_t *this,
                           void                    *ptr)
{
    lub_heap_status_t         status     = LUB_HEAP_CORRUPTED;
    lub_heap_cache_bucket_t **bucket_ptr = ptr;
    --bucket_ptr;
    if(*bucket_ptr == this)
    {
        lub_blockpool_free(&this->m_blockpool,bucket_ptr);
        /* make sure that released memory is tainted */
        lub_heap_taint_memory((char*)bucket_ptr,
                              LUB_HEAP_TAINT_FREE,
                              this->m_blockpool.m_block_size);
        status = LUB_HEAP_OK;
    }
    return status;
}
/*--------------------------------------------------------- */
