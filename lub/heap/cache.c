#include <stdio.h>
#include <string.h>
#include "cache.h"

/*--------------------------------------------------------- */
size_t
lub_heap_cache__get_max_free(lub_heap_cache_t *this)
{
    size_t size = 0;
    /* get the size from the cache */
    lub_heap_cache_bucket_t **ptr;

    for(ptr = this->m_bucket_start;
        ptr < this->m_bucket_end;
        ++ptr)
    {
        lub_blockpool_stats_t stats;
        lub_blockpool__get_stats(&(*ptr)->m_blockpool,&stats);
        if(stats.free_blocks && ((stats.block_size - sizeof(lub_heap_cache_bucket_t*)) > size))
        {
            size = stats.block_size - sizeof(lub_heap_cache_bucket_t*);
        }
    }
    return size;
}
/*--------------------------------------------------------- */
static lub_heap_cache_bucket_t *
_lub_heap_cache_find_bucket_from_size(lub_heap_cache_t *this,
                                      size_t            size)
{
    lub_heap_cache_bucket_t **ptr = this->m_bucket_start;
    /* leave space to put the bucket reference into place */
    size += sizeof(lub_heap_cache_bucket_t*);
    /* place powers of two into the correct bucket */
    --size; 
    /* ignore sizes of 1,2 and 4 bytes */
    size >>= 3; 
    for(;
        (ptr < this->m_bucket_end) && size;
        ++ptr)
    {
        size >>= 1;
    }
    return (ptr == this->m_bucket_end) ? 0 : *ptr;
}
/*--------------------------------------------------------- */
static size_t
lub_heap_cache_num_buckets(lub_heap_align_t max_block_size)
{
    unsigned num_buckets = 0;

    /* count the number of buckets to be created */
    max_block_size >>= 3; /* ignore buckets of size 1,2 and 4 bytes */
    while(max_block_size)
    {
        ++num_buckets;
        max_block_size >>= 1;
    }
    return num_buckets;
}
/*--------------------------------------------------------- */
size_t
lub_heap_cache_overhead_size(lub_heap_align_t max_block_size,
                             size_t           num_max_blocks)
{
    size_t overhead = 0;
    if(num_max_blocks && max_block_size)
    {
        /* 
         * account for the cache overheads
         */
        size_t num_buckets = lub_heap_cache_num_buckets(max_block_size);
        
        /* cache control block */
        overhead += sizeof(lub_heap_cache_t);
        
        /* array of bucket pointers (-1 because one is contained in control block) */
        overhead += sizeof(lub_heap_cache_bucket_t*) * (num_buckets-1);
        
        /* fast size lookup array */
        overhead += sizeof(lub_heap_cache_bucket_t*) * max_block_size;
        
        /* buckets themselves */
        overhead += num_buckets * offsetof(lub_heap_cache_bucket_t,m_memory_start);
    }
    return overhead;
}
/*--------------------------------------------------------- */
size_t
lub_heap_overhead_size(lub_heap_align_t max_block_size,
                       size_t           num_max_blocks)
{
    /* heap control block */
    size_t overhead = sizeof(lub_heap_t);
    
    /* need at least one free blocks worth */
    overhead += sizeof(lub_heap_free_block_t);

    if(max_block_size && num_max_blocks)
    {
        size_t num_buckets = lub_heap_cache_num_buckets(max_block_size);
        size_t bucket_size = max_block_size * num_max_blocks;

        /* now add any cache overhead contribution */
        overhead += lub_heap_cache_overhead_size(max_block_size,num_max_blocks);

        /* add the bucket contents */
        overhead += (num_buckets * bucket_size);
    }
    return overhead;
}
/*--------------------------------------------------------- */
lub_heap_status_t
lub_heap_cache_init(lub_heap_t      *this,
                    lub_heap_align_t max_block_size,
                    size_t           num_max_blocks)
{
    lub_heap_status_t status      = LUB_HEAP_FAILED;
    do
    {
        size_t                    bucket_size = (max_block_size * num_max_blocks);
        unsigned                  num_buckets = lub_heap_cache_num_buckets(max_block_size);
        size_t                    block_size  = 8;
        lub_heap_cache_t         *cache;
        lub_heap_cache_bucket_t **ptr;
        int                       i;
        /* cannot call this multiple times */
        if(this->cache) break;

        
        /* allocate a cache control block */
        cache = this->cache = lub_heap_static_alloc(this,
                                                    sizeof(lub_heap_cache_t) + sizeof(lub_heap_cache_bucket_t*)*(num_buckets-1));
        if(!cache) break;
        
        cache->m_max_block_size = max_block_size;
        cache->m_num_max_blocks = num_max_blocks;
        cache->m_num_buckets    = num_buckets;
        cache->m_bucket_size    = bucket_size;
        cache->m_misses         = 0;
        cache->m_bucket_end     = &cache->m_bucket_start[num_buckets];
    
        /* allocate each bucket for the cache */
        for(ptr = cache->m_bucket_start;
            ptr < cache->m_bucket_end;
            ++ptr)
        {
            *ptr = lub_heap_static_alloc(this,
                                         bucket_size + offsetof(lub_heap_cache_bucket_t,m_memory_start));
            if(!*ptr) break;
            /* set up each bucket */
            lub_heap_cache_bucket_init(*ptr,
                                       cache,
                                       block_size,
                                       bucket_size);
            block_size <<= 1;
        }
        if(ptr != cache->m_bucket_end) break;

        /* now create a fast lookup for resolving size to bucket */
        cache->m_bucket_lookup = lub_heap_static_alloc(this,sizeof(lub_heap_cache_bucket_t*)*max_block_size);
        if(!cache->m_bucket_lookup) break;

        for(i = 0;
            i < cache->m_max_block_size;
            ++i)
        {
            cache->m_bucket_lookup[i] = _lub_heap_cache_find_bucket_from_size(cache,i+1);
        }
        status = LUB_HEAP_OK;
    } while(0);
    
    return status;
}
/*--------------------------------------------------------- */
lub_heap_cache_bucket_t *
lub_heap_cache_find_bucket_from_address(lub_heap_cache_t *this,
                                        const void       *address)
{
    lub_heap_cache_bucket_t **bucket = (void*)address;
    
    /* get the bucket reference */
    --bucket;
    
    if(    (*bucket > this->m_bucket_start[0]) 
        || (*bucket < this->m_bucket_end[-1]) )
    {
        bucket = 0;
    }
    return bucket ? *bucket : 0;
}
/*--------------------------------------------------------- */
lub_heap_cache_bucket_t *
lub_heap_cache_find_bucket_from_size(lub_heap_cache_t *this,
                                     size_t            size)
{
    lub_heap_cache_bucket_t *bucket = 0;
    
    /* simply get the result from the fast lookup table */
    if(size > 0)
    {        
        if(size <= this->m_max_block_size)
        {
            bucket = this->m_bucket_lookup[size-1];
        }
        else
        {
            ++this->m_misses;
        }
    }
    return bucket;
}
/*--------------------------------------------------------- */
lub_heap_status_t
lub_heap_cache_realloc(lub_heap_t *this,
                       char      **ptr,
                       size_t      size)
{
    lub_heap_status_t        status     = LUB_HEAP_FAILED;
    lub_heap_cache_bucket_t *old_bucket = 0;
    lub_heap_cache_bucket_t *new_bucket = size ? lub_heap_cache_find_bucket_from_size(this->cache,size) : 0;
    char                    *new_ptr = 0;
    
    do
    {
        char *old_ptr = *ptr;
        if(old_ptr)
        {
            old_bucket = lub_heap_cache_find_bucket_from_address(this->cache,old_ptr);
        }
        else if (size == 0)
        {
            /* nothing to do */
            status = LUB_HEAP_OK;
            break;
        }
        if(old_bucket)
        {
            /********************************
             * old from CACHE, new from CACHE
             ******************************** */
            if(new_bucket)
            {
                if(old_bucket == new_bucket)
                {
                    /* keep the existing cache block which is big enough */
                    new_ptr = old_ptr;
                    status = LUB_HEAP_OK;
                    break;
                }
                /* try and get memory from the cache */
                new_ptr = lub_heap_cache_bucket_alloc(new_bucket);
                if(new_ptr)
                {
                    if(old_ptr)
                    {
                        /* copy the old details across */
                        memcpy(new_ptr,old_ptr,size);

                        /* release the old cache block */
                        status = lub_heap_cache_bucket_free(old_bucket,old_ptr);
                        if(LUB_HEAP_OK != status)
                        {
                            break;
                        }
                    }
                    /* all done */
                    status = LUB_HEAP_OK;
                    break;
                }
            }
            /********************************
             * old from CACHE, new from HEAP
             ******************************** */
            if(size)
            {
                /* get new memory from the dynamic heap */
                status = lub_heap_raw_realloc(this,&new_ptr,size,LUB_HEAP_ALIGN_NATIVE);
                if(LUB_HEAP_OK != status)
                {
                    break;
                }
                /* copy the old details across */
                memcpy(new_ptr,old_ptr,size);
            }
            /* release the old cache block */
            status = lub_heap_cache_bucket_free(old_bucket,old_ptr);
        }
        else
        {
            /********************************
             * old from HEAP, new from CACHE
             ******************************** */
            if(size)
            {
                if(new_bucket)
                {
                    /* try and get memory from the cache */
                    new_ptr = lub_heap_cache_bucket_alloc(new_bucket);
                    if(new_ptr)
                    {
                        if(old_ptr)
                        {
                            /* copy the old details across */
                            memcpy(new_ptr,old_ptr,size);
                            size = 0;
                        }
                        else
                        {
                            /* all done */
                            status = LUB_HEAP_OK;
                            break;
                        }
                    }
                }
            }
            else if(!old_ptr)
            {
                /* nothing to do */
                status = LUB_HEAP_OK;
                break;
            }
            /********************************
             * old from HEAP, new from HEAP
             ******************************** */
            /* release old memory and potentially get new memory */
            status = lub_heap_raw_realloc(this,&old_ptr,size,LUB_HEAP_ALIGN_NATIVE);
            if(LUB_HEAP_OK != status)
            {
                break;
            }
            else if(size)
            {
                new_ptr = old_ptr;
            }
        }
    } while(0);

    if(LUB_HEAP_OK == status)
    {
        /* set up the new pointer value */
        *ptr = new_ptr;
    }
    return status;
}
/*--------------------------------------------------------- */
void
lub_heap_cache_show(lub_heap_cache_t *this)
{
    lub_heap_cache_bucket_t **ptr;
        
    printf("       size num blocks    current  high-tide cumulative     misses\n");
    printf(" ---------- ---------- ---------- ---------- ---------- ----------\n");
    for(ptr = this->m_bucket_start;
        ptr < this->m_bucket_end;
        ++ptr)
    {
        lub_blockpool_t      *blockpool = &(*ptr)->m_blockpool;
        lub_blockpool_stats_t stats;
        
        lub_blockpool__get_stats(blockpool,&stats);

        printf(" %10"SIZE_FMT" %10"SIZE_FMT" %10"SIZE_FMT" %10"SIZE_FMT" %10"SIZE_FMT" %10"SIZE_FMT"\n",
               stats.block_size,
               stats.num_blocks,
               stats.alloc_blocks,
               stats.alloc_hightide_blocks,
               stats.alloc_total_blocks,
               stats.alloc_failures);
    }

    printf(" %10u %10s %10s %10s %10s %10"SIZE_FMT"\n",
           this->m_max_block_size,
           "-",
           "-",
           "-",
           "-",
          this->m_misses);
}
/*--------------------------------------------------------- */
void
lub_heap_cache__get_stats(lub_heap_cache_t *this,
                          lub_heap_stats_t *stats)
{
    lub_heap_cache_bucket_t **ptr;

    memset(stats,0,sizeof(lub_heap_stats_t));
    stats->free_overhead = lub_heap_cache_overhead_size(this->m_max_block_size,this->m_num_max_blocks);

    for(ptr = this->m_bucket_start;
        ptr < this->m_bucket_end;
        ++ptr)
    {
        lub_blockpool_t      *blockpool = &(*ptr)->m_blockpool;
        lub_blockpool_stats_t bucket_stats;
        size_t                block_size;
        size_t                block_overhead = sizeof(lub_heap_cache_bucket_t*); /* used for fast lookup from address */
        
        lub_blockpool__get_stats(blockpool,&bucket_stats);
        block_size = (bucket_stats.block_size - block_overhead);

        stats->free_blocks   += bucket_stats.free_blocks;
        stats->free_bytes    += block_size * bucket_stats.free_blocks;
        stats->free_overhead += block_overhead * bucket_stats.free_blocks;

        stats->alloc_blocks   += bucket_stats.alloc_blocks;
        stats->alloc_bytes    += block_size * bucket_stats.alloc_blocks;
        stats->alloc_overhead += block_overhead * bucket_stats.alloc_blocks;

        stats->alloc_total_blocks += bucket_stats.alloc_total_blocks;
        stats->alloc_total_bytes  += block_size * bucket_stats.alloc_total_blocks;
    }
    
}
/*--------------------------------------------------------- */
