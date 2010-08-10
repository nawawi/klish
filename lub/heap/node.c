/*
 * node.c
 */
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "private.h"
#include "context.h"
#include "node.h"

int lub_heap_leak_verbose = 0;
/*--------------------------------------------------------- */
int
lub_heap_node_compare(const void *clientnode,
                      const void *clientkey)
{
    const lub_heap_node_t     * node = clientnode;
    const lub_heap_node_key_t * key  = clientkey;
    
    return ((long)node - (long)key->node);
}
/*--------------------------------------------------------- */
/* we simply use the node address as the index */
void
lub_heap_node_getkey(const void        *clientnode,
                    lub_bintree_key_t *key)
{
    lub_heap_node_key_t clientkey;
    clientkey.node = clientnode;
    memcpy(key,&clientkey,sizeof(lub_heap_node_key_t));
}
/*--------------------------------------------------------- */
static void
lub_heap_node_meta_init(void)
{
    static bool_t initialised = BOOL_FALSE;
    
    if(BOOL_FALSE == initialised)
    {
        lub_heap_leak_t *leak = lub_heap_leak_instance();
        initialised = BOOL_TRUE;
        /* initialise the node tree */
        lub_bintree_init(&leak->m_node_tree,
                         offsetof(lub_heap_node_t,bt_node),
                         lub_heap_node_compare,
                         lub_heap_node_getkey);
        /* initialise the clear node tree */
        lub_bintree_init(&leak->m_clear_node_tree,
                         offsetof(lub_heap_node_t,bt_node),
                         lub_heap_node_compare,
                         lub_heap_node_getkey);
        lub_heap_leak_release(leak);
    }
}
/*--------------------------------------------------------- */
void
lub_heap_node_init(lub_heap_node_t    * this,
                   lub_heap_context_t * context)
{
    lub_heap_node_meta_init();
    
    /* initialise the control blocks */
    lub_bintree_node_init(&this->bt_node);

    lub_heap_node__set_context(this,context);
    lub_heap_node__set_leaked(this,BOOL_FALSE);
    lub_heap_node__set_partial(this,BOOL_FALSE);
    lub_heap_node__set_next(this,NULL);
    lub_heap_node__set_scanned(this,BOOL_FALSE);
    this->prev = NULL;
    {
        lub_heap_leak_t *leak = lub_heap_leak_instance();
        /* add this to the node tree */
        lub_bintree_insert(context ? &leak->m_node_tree : &leak->m_clear_node_tree,this);
        lub_heap_leak_release(leak);
    }
    if(context)
    {
        /* add ourselves to this context */
        lub_heap_context_insert_node(context,this);

        /* maintain the leak statistics */
        ++context->allocs;
        context->alloc_bytes    += lub_heap_node__get_size(this);
        context->alloc_overhead += lub_heap_node__get_overhead(this);
    }
}
/*--------------------------------------------------------- */
void
lub_heap_node_fini(lub_heap_node_t * this)
{
    /* remove this node from this context */
    lub_heap_node_clear(this,0);
    {
        lub_heap_leak_t *leak = lub_heap_leak_instance();
        /* remove this from the clear_node tree */
        lub_bintree_remove(&leak->m_clear_node_tree,this);
        lub_heap_leak_release(leak);
    }
}
/*--------------------------------------------------------- */
size_t
    lub_heap_node__get_instanceSize(void)
{
    return sizeof(lub_heap_node_t);
}
/*--------------------------------------------------------- */
size_t
    lub_heap_node__get_size(const lub_heap_node_t *this)
{
    size_t size = lub_heap__get_block_size(lub_heap_node__get_context(this)->heap,this);
    size -= sizeof(lub_heap_node_t);
    return size;
}
/*--------------------------------------------------------- */
size_t
    lub_heap_node__get_overhead(const lub_heap_node_t *this)
{
    size_t overhead;
    
    overhead = lub_heap__get_block_overhead(lub_heap_node__get_context(this)->heap,
                                            lub_heap_node__get_ptr(this));
    
    overhead += sizeof(lub_heap_node_t);
    return overhead;
}
/*--------------------------------------------------------- */
char *
lub_heap_node__get_ptr(const lub_heap_node_t *this)
{
    return (char*)&this[1];
}
/*--------------------------------------------------------- */
bool_t 
lub_heap_validate_pointer(lub_heap_t *this,
                          char       *ptr)
{
    bool_t result = BOOL_FALSE;
    do
    {
        lub_heap_segment_t *seg;
        if((0 < lub_heap_frame_count))
        {
            /*
             * When leak detection is enabled we can perform detailed 
             * validation of pointers.
             */
            lub_heap_node_t *node = lub_heap_node_from_start_of_block_ptr(ptr);
            if(NULL != node)
            {
                lub_heap_context_t *context = lub_heap_node__get_context(node);
                /* we've found a context so can give a definative answer */
                if(this == context->heap)
                {
                    result = BOOL_TRUE;
                }
                break;
            }
        }
        /* iterate around each of the segments belonging to this heap */
        for(seg = &this->first_segment;
            seg;
            seg = seg->next)
        {
            char *start = (char*)&seg[1];
            if(ptr >= start)
            {
                char *end = start + (seg->words << 2);
                if(ptr < end)
                {
                    /* we've found a parent segment for this pointer */
                    result = BOOL_TRUE;
                    break;
                }
            }
        }
    } while(0);
    ptr = 0; /* don't leave a pointer on the stack */
    return result;
}
/*--------------------------------------------------------- */
static lub_heap_node_t *
_lub_heap_node_from_ptr(lub_bintree_t *tree,
                        const char    *ptr, 
                        bool_t         start_of_block)
{
    lub_heap_node_t *result = 0;
    if(tree->root)
    {
        lub_heap_node_key_t key;
        
        /* search for the node which comes immediately before this pointer */
        key.node = (lub_heap_node_t *)ptr;
        result = lub_bintree_findprevious(tree,&key);
    
        if(NULL != result)
        {
            char *tmp = lub_heap_node__get_ptr(result);
            /* ensure that the pointer is within the scope of this node */
            if(start_of_block)
            {
                if(ptr != tmp)
                {
                    /* 
                     * this should be an exact match and isn't
                     */
                    result = NULL;
                }
            }
            else if(ptr < tmp)
            {
                /* 
                 * this is referencing part of the node header
                 */
                result = NULL;
            }
            else
            {
                /* compare with the end of the allocated memory */
                tmp += lub_heap_node__get_size(result);
                /* ensure that the pointer is within the scope of this node */
                if(ptr >= tmp)
                {
                    /* out of range of this node */
                    result = NULL;
                }
            }
            tmp = 0; /* don't leave a pointer on the stack */
        }
    }
    ptr = 0; /* don't leave a pointer on the stack */
    return result;
}
/*--------------------------------------------------------- */
lub_heap_node_t *
lub_heap_node_from_start_of_block_ptr(char *ptr)
{
    lub_heap_node_t *result = 0;
    if(lub_heap_leak_query_node_tree())
    {
        lub_heap_leak_t *leak = lub_heap_leak_instance();
        result = _lub_heap_node_from_ptr(&leak->m_node_tree,ptr,BOOL_TRUE);
        lub_heap_leak_release(leak);
    }
    if((0 == result) && lub_heap_leak_query_clear_node_tree())
    {
        lub_heap_leak_t *leak = lub_heap_leak_instance();
        result = _lub_heap_node_from_ptr(&leak->m_clear_node_tree,ptr,BOOL_TRUE);
        lub_heap_leak_release(leak);
    }
    ptr = 0; /* don't leave a pointer on the stack */
    return result;
}
/*--------------------------------------------------------- */
void
lub_heap_node__set_context(lub_heap_node_t    *this,
                        lub_heap_context_t *value)
{
    unsigned long mask = ((unsigned long)value & ~(LEAKED_MASK | PARTIAL_MASK));
    this->_context = (lub_heap_context_t *)mask;
}
/*--------------------------------------------------------- */
lub_heap_context_t *
lub_heap_node__get_context(const lub_heap_node_t *this)
{
    unsigned long mask = (unsigned long)this->_context & ~(LEAKED_MASK | PARTIAL_MASK);
    return (lub_heap_context_t *)mask;
}
/*--------------------------------------------------------- */
void
lub_heap_node__set_next(lub_heap_node_t *this,
                     lub_heap_node_t *value)
{
    unsigned long mask = ((unsigned long)value & ~(SCANNED_MASK));
    this->_next = (lub_heap_node_t *)mask;
}
/*--------------------------------------------------------- */
lub_heap_node_t *
lub_heap_node__get_next(const lub_heap_node_t *this)
{
    unsigned long mask = (unsigned long)this->_next & ~(SCANNED_MASK);
    return (lub_heap_node_t *)mask;
}
/*--------------------------------------------------------- */
bool_t
lub_heap_node__get_leaked(const lub_heap_node_t *this)
{
    return ((unsigned long)this->_context & LEAKED_MASK) ? BOOL_TRUE : BOOL_FALSE;
}
/*--------------------------------------------------------- */
bool_t
lub_heap_node__get_partial(const lub_heap_node_t *this)
{
    return ((unsigned long)this->_context & PARTIAL_MASK) ? BOOL_TRUE : BOOL_FALSE;
}
/*--------------------------------------------------------- */
bool_t
lub_heap_node__get_scanned(const lub_heap_node_t *this)
{
    return ((unsigned long)this->_next & SCANNED_MASK) ? BOOL_TRUE : BOOL_FALSE;
}
/*--------------------------------------------------------- */
void
lub_heap_node__set_leaked(lub_heap_node_t *this, bool_t value)
{
    unsigned long mask = (unsigned long)this->_context;
    if(BOOL_TRUE == value)
    {
        mask |= LEAKED_MASK;
    }
    else
    {
        mask &= ~LEAKED_MASK;
    }
    this->_context = (lub_heap_context_t *)mask;
}
/*--------------------------------------------------------- */
void
lub_heap_node__set_partial(lub_heap_node_t *this, bool_t value)
{
    unsigned long mask = (unsigned long)this->_context;
    if(BOOL_TRUE == value)
    {
        mask |= PARTIAL_MASK;
    }
    else
    {
        mask &= ~PARTIAL_MASK;
    }
    this->_context = (lub_heap_context_t *)mask;
}
/*--------------------------------------------------------- */
void
lub_heap_node__set_scanned(lub_heap_node_t *this, bool_t value)
{
    unsigned long mask = (unsigned long)this->_next;
    if(BOOL_TRUE == value)
    {
        mask |= SCANNED_MASK;
    }
    else
    {
        mask &= ~SCANNED_MASK;
    }
    this->_next = (lub_heap_node_t *)mask;
}
/*--------------------------------------------------------- */
void
lub_heap_foreach_node(void (*fn)(lub_heap_node_t *,void*),
                      void  *arg)
{
    lub_heap_leak_t     *leak = lub_heap_leak_instance();
    lub_heap_node_t     *node;
    lub_heap_node_key_t  key;

    for(node = lub_bintree_findfirst(&leak->m_node_tree);
        node;
        node = lub_bintree_findnext(&leak->m_node_tree,&key))
    {
        lub_heap_leak_release(leak);

        lub_heap_node_getkey(node,(lub_bintree_key_t*)&key);
        /* invoke the specified method on this node */
        fn(node,arg);

        leak = lub_heap_leak_instance();
    }
    lub_heap_leak_release(leak);
}
/*--------------------------------------------------------- */
void
lub_heap_node_clear(lub_heap_node_t *this,
                    void            *arg)
{
    /* clearing a node removes it from it's context */
    lub_heap_context_t *context = lub_heap_node__get_context(this);
    if(NULL != context)
    {
        lub_heap_leak_t *leak = lub_heap_leak_instance();
        /* remove from the node tree and place into the clear tree */
        lub_bintree_remove(&leak->m_node_tree,this);
        lub_bintree_insert(&leak->m_clear_node_tree,this);
        lub_heap_leak_release(leak);

        /* maintain the leak statistics */
        --context->allocs;
        context->alloc_bytes    -= lub_heap_node__get_size(this);
        context->alloc_overhead -= lub_heap_node__get_overhead(this);

        lub_heap_node__set_context(this,NULL);
        lub_heap_node__set_leaked(this,BOOL_FALSE);
        lub_heap_node__set_partial(this,BOOL_FALSE);

        lub_heap_context_remove_node(context,this);
    }
}
/*--------------------------------------------------------- */
#define RELEASE_COUNT 2048
void 
lub_heap_scan_memory(const void  *mem,
                     size_t       size)
{
    size_t              bytes_left = size;
    typedef const void *void_ptr;
    void_ptr           *ptr,last_ptr = 0;
    lub_heap_leak_t    *leak = lub_heap_leak_instance();
    unsigned            release_count = RELEASE_COUNT;
    if(lub_heap_leak_verbose)
    {
        printf("lub_heap_scan_memory(%p,%"SIZE_FMT")\n",mem,size);
    }
    ++leak->m_stats.scanned;
    
    /* scan all the words in this allocated block of memory */
    for(ptr = (void_ptr*)mem;
        bytes_left;
        )
    {
        if(0 == --release_count)
        {
            /* make space for other memory tasks to get in... */
            lub_heap_leak_release(leak);
            leak          = lub_heap_leak_instance();
            release_count = RELEASE_COUNT;
        }
        if(*ptr != last_ptr)
        {
            lub_heap_node_t    *node;
            lub_heap_node_key_t key;
            
            /* search for a node */
            key.node = (lub_heap_node_t *)ptr;
            node = lub_bintree_find(&leak->m_node_tree,&key);
            if(NULL != node)
            {
                /*
                 * If we stumble across a node don't scan it's contents as this could cause
                 * false negatives. This situation could occur if an allocated block of memory
                 * was used as a heap in it's own right.
                 */
                char  *tmp       = lub_heap_node__get_ptr(node);
                size_t node_size = lub_heap_node__get_size(node);
                
                /* skip forward past the node contents */
                ptr         = (void_ptr*)(tmp + node_size);
                bytes_left -= (node_size + sizeof(lub_heap_node_t));
                tmp = 0; /* don't leave pointers on our stack */
                last_ptr = 0;
                continue;
            }
            /* 
             * see whether this is a reference to a node 
             * NB. we only resolve a node if the pointer lies
             * within the memory allocated to the client; any direct
             * references to nodes will not resolve to a node
             * object.
             */
            node = _lub_heap_node_from_ptr(&leak->m_node_tree,*ptr,BOOL_FALSE);
            if( (NULL != node))
            {
                /* this looks like a valid node */
                lub_heap_context_t *context       = lub_heap_node__get_context(node);
                size_t              node_size     = lub_heap_node__get_size(node);
                size_t              node_overhead = lub_heap_node__get_overhead(node);
                char               *tmp           = lub_heap_node__get_ptr(node);
                if(tmp == *ptr)
                {
                    /* reference to start of block */
                    if(BOOL_TRUE == lub_heap_node__get_partial(node))
                    {
                        lub_heap_node__set_partial(node,BOOL_FALSE);
    
                        if(NULL != context)
                        {
                            --context->partials;
                            context->partial_bytes    -= node_size;
                            context->partial_overhead -= node_overhead;
                        }
                    }
                }
                tmp = 0; /* don't leave pointers on our stack */
                /* this is definately not a leak */
                if(BOOL_TRUE == lub_heap_node__get_leaked(node))
                {
                    lub_heap_node__set_leaked(node,BOOL_FALSE);
    
                    if(NULL != context)
                    {
                        --context->leaks;
                        context->leaked_bytes    -= node_size;
                        context->leaked_overhead -= node_overhead;
     
                        --leak->m_stats.leaks;
                        leak->m_stats.leaked_bytes    -= node_size;
                        leak->m_stats.leaked_overhead -= node_overhead;
                    }
                }
            }
        }
        last_ptr = *ptr++;
        bytes_left -= sizeof(void*);
    }
    lub_heap_leak_release(leak);
    last_ptr = ptr = 0; /* don't leave pointers on the stack */
}
/*--------------------------------------------------------- */
