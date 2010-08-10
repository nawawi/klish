#include "lub/blockpool.h"
#include "private.h"

typedef struct _lub_heap_cache_bucket lub_heap_cache_bucket_t;
/*-------------------------------------
 * lub_heap_cache_t class
 *------------------------------------- */
struct _lub_heap_cache
{
    lub_heap_align_t          m_max_block_size;
    unsigned                  m_num_max_blocks;
    unsigned                  m_num_buckets;
    size_t                    m_bucket_size;
    size_t                    m_misses;
    lub_heap_cache_bucket_t **m_bucket_lookup;
    lub_heap_cache_bucket_t **m_bucket_end;
    lub_heap_cache_bucket_t  *m_bucket_start[1]; /* must be last */
};

lub_heap_cache_bucket_t *
    lub_heap_cache_find_bucket_from_address(lub_heap_cache_t *instance,
                                            const void       *address);
lub_heap_cache_bucket_t *
    lub_heap_cache_find_bucket_from_size(lub_heap_cache_t *instance,
                                         size_t            size);
void
    lub_heap_cache__get_stats(lub_heap_cache_t *this,
                              lub_heap_stats_t *stats);

/*-------------------------------------
 * lub_heap_cache_bucket_t class
 *------------------------------------- */
struct _lub_heap_cache_bucket
{
    lub_blockpool_t   m_blockpool;
    lub_heap_cache_t *m_cache;
    char             *m_memory_end;
    char              m_memory_start[1]; /* must be last */
};

void
    lub_heap_cache_bucket_init(lub_heap_cache_bucket_t *instance,
                               lub_heap_cache_t        *cache,
                               size_t                   block_size,
                               size_t                   bucket_size);
void *
    lub_heap_cache_bucket_alloc(lub_heap_cache_bucket_t *instance);
lub_heap_status_t
    lub_heap_cache_bucket_free(lub_heap_cache_bucket_t *instance,
                                void                    *ptr);
size_t
    lub_heap_cache__get_max_free(lub_heap_cache_t *this);

lub_heap_cache_bucket_t *
    lub_heap_cache_find_bucket_from_address(lub_heap_cache_t *this,
                                            const void       *address);
size_t
    lub_heap_cache_bucket__get_block_overhead(lub_heap_cache_bucket_t *this,
                                              const char              *block);
size_t
    lub_heap_cache_bucket__get_block_size(lub_heap_cache_bucket_t *this,
                                          const char              *block);
