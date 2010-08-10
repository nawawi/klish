/*
 * heap_add_segment.c
 */
#include <stdlib.h>
#include <string.h>

#include "private.h"
#include "context.h"
#include "lub/bintree.h"

typedef struct
{
    const lub_heap_segment_t *segment;
} lub_heap_segment_key_t;

/*--------------------------------------------------------- */
static int
lub_heap_segment_compare(const void *clientnode,
                         const void *clientkey)
{
    const lub_heap_segment_t     * segment = clientnode;
    const lub_heap_segment_key_t * key     = clientkey;
    
    return ((long)segment - (long)key->segment);
}
/*--------------------------------------------------------- */
/* we simply use the segment address as the index */
static void
lub_heap_segment_getkey(const void        *clientnode,
                        lub_bintree_key_t *key)
{
    lub_heap_segment_key_t clientkey;
    clientkey.segment = clientnode;
    memcpy(key,&clientkey,sizeof(lub_heap_segment_key_t));
}
/*--------------------------------------------------------- */
static void
lub_heap_segment_meta_init()
{
    static bool_t initialised = BOOL_FALSE;
    
    if(BOOL_FALSE == initialised)
    {
        lub_heap_leak_t *leak = lub_heap_leak_instance();
        initialised = BOOL_TRUE;
        /* initialise the segment tree */
        lub_bintree_init(&leak->m_segment_tree,
                         offsetof(lub_heap_segment_t,bt_node),
                         lub_heap_segment_compare,
                         lub_heap_segment_getkey);
        lub_heap_leak_release(leak);
    }
}
/*--------------------------------------------------------- */
const lub_heap_segment_t *
lub_heap_segment_findnext(const void *address)
{
    lub_heap_segment_t    *segment;
    lub_heap_segment_key_t key;
    lub_heap_leak_t       *leak = lub_heap_leak_instance();
    key.segment = address;
    
    segment = lub_bintree_findnext(&leak->m_segment_tree,&key);
    lub_heap_leak_release(leak);

    return segment;
}
/*--------------------------------------------------------- */
/*
 * as an optimisation this could be modified to merge 
 * adjacent segments together
 */
void
lub_heap_add_segment(lub_heap_t *this,
                     void       *start,
                     size_t      size)
{
    lub_heap_segment_t *segment;
    
    /* check for simple adjacent segment as produced by sbrk() type behaviour */
    if(this->first_segment.words)
    {
        lub_heap_tag_t *tail;
        char           *ptr = (char*)&this[1];
        ptr += (this->first_segment.words << 2);
        if(ptr == start)
        {
            /* simply extend the current first segment */
            this->first_segment.words += (size >> 2);
            tail          = &((lub_heap_tag_t*)start)[-1];
            tail->segment = BOOL_FALSE; /* no longer last block in segment */
            /* convert the new memory into a free block... */
            lub_heap_init_free_block(this,start,size,BOOL_FALSE,BOOL_TRUE);
            /* try and merge with the previous block */
            lub_heap_merge_with_previous(this,start);

            this->stats.segs_bytes    += size;

            return;
        }
    }
    /* adjust the size which can be used */
    size -= sizeof(lub_heap_segment_t);

    /* set up the linked list of segments */
    segment = start;
    
    /* update the stats */
    ++this->stats.segs;
    this->stats.segs_bytes    += size;
    this->stats.segs_overhead += sizeof(lub_heap_segment_t);
    
    if(segment != &this->first_segment)
    {
        /* maintain the list of segments */
        segment->next = this->first_segment.next;
        this->first_segment.next = segment;
    }
    segment->words = (size >> 2);
    
    lub_heap_init_free_block(this,&segment[1],size,BOOL_TRUE,BOOL_TRUE);
    
    /* add this segment to the tree */
    lub_heap_segment_meta_init();
    lub_bintree_node_init(&segment->bt_node);
    {
        lub_heap_leak_t *leak = lub_heap_leak_instance();
        lub_bintree_insert(&leak->m_segment_tree,segment);
        lub_heap_leak_release(leak);
    }
}
/*--------------------------------------------------------- */
