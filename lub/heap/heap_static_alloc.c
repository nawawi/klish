#include "cache.h"

/*--------------------------------------------------------- */
size_t
lub_heap__get_block_overhead(lub_heap_t *this,
                             const void *block)
{
    size_t                   overhead;
    lub_heap_cache_bucket_t *bucket = 0;
    if(this->cache)
    {
        bucket = lub_heap_cache_find_bucket_from_address(this->cache,block);
    }
    if(bucket)
    {
        overhead = lub_heap_cache_bucket__get_block_overhead(bucket,block);
    }
    else
    {
        overhead = sizeof(lub_heap_alloc_block_t);
    }
    return overhead;
}
/*--------------------------------------------------------- */
size_t
lub_heap__get_block_size(lub_heap_t *this,
                         const void *block)
{
    size_t                   bytes = 0;
    lub_heap_cache_bucket_t *bucket = 0;
    if(this->cache)
    {
        bucket = lub_heap_cache_find_bucket_from_address(this->cache,block);
    }
    if(bucket)
    {
        bytes = lub_heap_cache_bucket__get_block_size(bucket,block);
    }
    else
    {
        const lub_heap_tag_t *header = block;
        --header;
        bytes = (header->words << 2);
        bytes -= sizeof(lub_heap_alloc_block_t);
    }
    return bytes;
}
/*--------------------------------------------------------- */
void *
lub_heap_static_alloc(lub_heap_t *this,
                      size_t      requested_size)
{
    void                  *result = NULL;
    lub_heap_free_block_t *free_block;
    /* round up to native alignment */
    words_t                words;
    lub_heap_key_t         key;
    size_t                 size = requested_size;
    
    size  = (size + (LUB_HEAP_ALIGN_NATIVE-1))/LUB_HEAP_ALIGN_NATIVE; 
    size *= LUB_HEAP_ALIGN_NATIVE; 
    words = (size >> 2);
    
    /* initialise the search key */
    key.words   = words;
    key.block   = 0;
    
    /* search for the start of a segment which is large enough for the request */
    while( (free_block = lub_bintree_findnext(&this->free_tree,&key)) )
    {
        lub_heap_tag_t *tail = lub_heap_block__get_tail((lub_heap_block_t*)free_block);
        
        /* is this free block at the start of a segment? */
        if(1 == tail->segment)
        {
            /* yes this is the end of a segment */
            break;
        }
        /* update the search key */
        lub_heap_block_getkey(free_block,(lub_bintree_key_t*)&key);
    }
    if(NULL != free_block)
    {
        /* remove this block from the free tree */
        lub_bintree_remove(&this->free_tree,free_block);
        
        /* 
         * get some memory from the bottom of this free block 
         */
        result = lub_heap_slice_from_top(this,
                                         &free_block,
                                         &words,
                                         BOOL_TRUE);

        if(NULL != free_block)
        {
            /* put the free block back into the tree */
            lub_bintree_insert(&this->free_tree,free_block);
        }
        if(NULL != result)
        {
            size_t allocated_size = (words << 2);
            /* update the stats */
            ++this->stats.static_blocks;
            this->stats.static_bytes    += size;
            this->stats.static_overhead += (allocated_size - size); 
        }
    }
    return result;
}
/*--------------------------------------------------------- */
