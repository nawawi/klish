/*
 * heap_create.c
 */
#include "private.h"
#include "context.h"

/*--------------------------------------------------------- */
static int
lub_heap_block_compare(const void *clientnode,
                       const void *clientkey)
{
    int                           delta;
    const lub_heap_free_block_t *block = clientnode;
    const lub_heap_key_t        *key   = clientkey;
    
    delta = (block->tag.words - key->words);
    if(0 == delta)
    {
        /* if same size differentiate by address */
        delta = (block - key->block);
    }
    return delta;
}
/*--------------------------------------------------------- */
lub_heap_t *
lub_heap_create(void  *start,
                size_t size)
{
    lub_heap_t *this = NULL;
    
    /* we must have at least 1024 bytes for a heap */
    if(size > (sizeof(lub_heap_t) + 4))
    {
        this = start;
        /* set up the binary tree for the free blocks */
        lub_bintree_init(&this->free_tree,
                         offsetof(lub_heap_free_block_t,bt_node),
                         lub_heap_block_compare,
                         lub_heap_block_getkey);

        this->cache    = 0;
        this->suppress = 0;
        
        /* initialise the statistics */
        this->stats.segs                    = 0;
        this->stats.segs_bytes              = 0;
        this->stats.segs_overhead           = 0;
        this->stats.free_blocks             = 0;
        this->stats.free_bytes              = 0;
        this->stats.free_overhead           = 0;
        this->stats.static_blocks           = 0;
        this->stats.static_bytes            = 0;
        this->stats.static_overhead         = 0;
        this->stats.alloc_blocks            = 0;
        this->stats.alloc_bytes             = 0;
        this->stats.alloc_overhead          = 0;
        this->stats.alloc_hightide_blocks   = 0;
        this->stats.alloc_hightide_bytes    = 0;
        this->stats.alloc_hightide_overhead = 0;
        this->stats.free_hightide_blocks    = 0;
        this->stats.free_hightide_bytes     = 0;
        this->stats.free_hightide_overhead  = 0;
        this->stats.alloc_total_blocks      = 0;
        this->stats.alloc_total_bytes       = 0;
                
        /* initialise the first segment */
        this->first_segment.next = NULL;
        
        /* create the default segment */
        lub_heap_add_segment(this, 
                             &this->first_segment, 
                             size - sizeof(lub_heap_t) + sizeof(lub_heap_segment_t));
        
        this->stats.segs_overhead += sizeof(lub_heap_t);
                            
        /* add this heap to the linked list of heaps in the system */
        {
            lub_heap_leak_t *leak = lub_heap_leak_instance();
            this->next            = leak->m_heap_list;
            leak->m_heap_list = this;
            lub_heap_leak_release(leak);
        }
    }
    return this;
}
/*--------------------------------------------------------- */
