#include <string.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "private.h"
#include "context.h"
#include "node.h"

unsigned long lub_heap_frame_count;

#define CONTEXT_CHUNK_SIZE 100 /* number of contexts per chunk */
#define CONTEXT_MAX_CHUNKS 100 /* allow upto 10000 contexts */

/*--------------------------------------------------------- 
 * PRIVATE META FUNCTIONS
 *--------------------------------------------------------- */
void
lub_heap_context_meta_init(void)
{
    static bool_t initialised = BOOL_FALSE;
    if(BOOL_FALSE == initialised)
    {
        lub_heap_leak_t *leak = lub_heap_leak_instance();
        initialised = BOOL_TRUE;

        /* initialise the context tree */
        lub_bintree_init(&leak->m_context_tree,
                         offsetof(lub_heap_context_t,bt_node),
                         lub_heap_context_compare,
                         lub_heap_context_getkey);
        lub_heap_leak_release(leak);
    }
}
/*--------------------------------------------------------- */
int
lub_heap_context_compare(const void *clientnode,
                         const void *clientkey)
{
    int                                   i,delta=0;
    const lub_heap_context_t             *node = clientnode;
    const lub_heap_context_key_t         *key  = clientkey;    
    function_t                   * const *node_fn = node->key.backtrace;
    function_t                   * const *key_fn  = key->backtrace;
        
    for(i = lub_heap_frame_count;
        i;
        --i,++node_fn,++key_fn)
    {
        delta = ((unsigned long)*node_fn - (unsigned long)*key_fn);
        if(0 != delta)
        {
            break;
        }
    }
    return delta;
}
/*--------------------------------------------------------- */
/* we simply use the embedded key as the index */
void
lub_heap_context_getkey(const void        *clientnode,
                        lub_bintree_key_t *key)
{
    const lub_heap_context_t * context = clientnode;
    memcpy(key,&context->key,sizeof(lub_heap_context_key_t));
}
/*--------------------------------------------------------- */
static bool_t
lub_heap_foreach_context(bool_t (*fn)(lub_heap_context_t *,void *),
                         void    *arg)
{
    bool_t                  result = BOOL_FALSE;
    lub_heap_context_t    * context;
    lub_bintree_iterator_t  iter;

    lub_heap_context_meta_init();

    {
        lub_heap_leak_t *leak = lub_heap_leak_instance();
        for(context = lub_bintree_findfirst(&leak->m_context_tree),
            context ? lub_bintree_iterator_init(&iter,&leak->m_context_tree,context) : (void)0;
            context;
            context = lub_bintree_iterator_next(&iter))
        {
            lub_heap_leak_release(leak);
            /* invoke the specified method on this context */
            result = fn(context,arg);
            leak = lub_heap_leak_instance();
        }
        lub_heap_leak_release(leak);
    }
    return result;
}
/*--------------------------------------------------------- */
static void
lub_heap_show_summary(void)
{
    lub_heap_leak_t *leak = lub_heap_leak_instance();
    size_t ok_allocs   = leak->m_stats.allocs;
    size_t ok_bytes    = leak->m_stats.alloc_bytes; 
    size_t ok_overhead = leak->m_stats.alloc_overhead; 
    
    ok_allocs -= leak->m_stats.partials;
    ok_allocs -= leak->m_stats.leaks;
    
    ok_overhead -= leak->m_stats.partial_overhead;
    ok_overhead -= leak->m_stats.leaked_overhead;
    
    printf("\n"
           "          +----------+----------+----------+----------+\n");
    printf("  TOTALS  |    blocks|     bytes|   average|  overhead|\n");
    printf("+---------+----------+----------+----------+----------+\n");
    printf("|contexts |%10"SIZE_FMT"|%10"SIZE_FMT"|%10"SIZE_FMT"|%10s|\n",
            leak->m_stats.contexts,
            leak->m_stats.contexts * sizeof(lub_heap_context_t),
            leak->m_stats.contexts ? sizeof(lub_heap_context_t) : 0,
            "");
    printf("|allocs   |%10"SIZE_FMT"|%10"SIZE_FMT"|%10"SIZE_FMT"|%10"SIZE_FMT"|\n",
            ok_allocs,
            ok_bytes,
            ok_allocs ? (ok_bytes / ok_allocs) : 0,
            ok_overhead);
    if(leak->m_stats.partials)
    {
        printf("|partials |%10"SIZE_FMT"|%10"SIZE_FMT"|%10"SIZE_FMT"|%10"SIZE_FMT"|\n",
                leak->m_stats.partials,
                leak->m_stats.partial_bytes,
                leak->m_stats.partials ? (leak->m_stats.partial_bytes / leak->m_stats.partials) : 0,
                leak->m_stats.partial_overhead);
    }
    if(leak->m_stats.leaks)
    {
        printf("|leaks    |%10"SIZE_FMT"|%10"SIZE_FMT"|%10"SIZE_FMT"|%10"SIZE_FMT"|\n",
                leak->m_stats.leaks,
                leak->m_stats.leaked_bytes,
                leak->m_stats.leaks ? (leak->m_stats.leaked_bytes / leak->m_stats.leaks) : 0,
                leak->m_stats.leaked_overhead);
    }
    printf("+---------+----------+----------+----------+----------+\n");
    lub_heap_leak_release(leak);
}
/*--------------------------------------------------------- */

/*--------------------------------------------------------- 
 * PRIVATE METHODS
 *--------------------------------------------------------- */
static void
lub_heap_context_foreach_node(lub_heap_context_t *this,
                              void           (*fn)(lub_heap_node_t *))
{
    lub_heap_node_t * node;
 
    for(node = this->first_node;
        node;
        node = lub_heap_node__get_next(node))
    {
        /* invoke the specified method on this node */
        fn(node);
    }
}
/*--------------------------------------------------------- */

/*--------------------------------------------------------- 
 * PUBLIC METHODS
 *--------------------------------------------------------- */
void
lub_heap_context_init(lub_heap_context_t * this,
                      lub_heap_t         * heap,
                      const stackframe_t * stack)
{
    lub_heap_context_meta_init();
    
    this->heap             = heap;
    this->allocs           = 0;
    this->alloc_bytes      = 0;
    this->alloc_overhead   = 0;
    this->leaks            = 0;
    this->leaked_bytes     = 0;
    this->leaked_overhead  = 0;
    this->partials         = 0;
    this->partial_bytes    = 0;
    this->partial_overhead = 0;
    this->first_node       = NULL;
    
    /* set the current backtrace for the context */
    this->key = *stack;
    
    /* intialise this context's binary tree node */
    lub_bintree_node_init(&this->bt_node);

    {
        lub_heap_leak_t *leak = lub_heap_leak_instance();
        /* add this context to the context_tree */
        lub_bintree_insert(&leak->m_context_tree,this);
        lub_heap_leak_release(leak);
    }
}
/*--------------------------------------------------------- */
void
lub_heap_context_fini(lub_heap_context_t * this)
{
    lub_heap_leak_t *leak = lub_heap_leak_instance();
    /* remove this node from the context_tree */
    lub_bintree_remove(&leak->m_context_tree,this);
    lub_heap_leak_release(leak);

    /* cleanup the context */
    lub_heap_context_clear(this);
}
/*--------------------------------------------------------- */
void
lub_heap_context_insert_node(lub_heap_context_t * this,
                             lub_heap_node_t    * node)
{
    /* add the node to the linked list */
    lub_heap_node__set_next(node,this->first_node);
    node->prev = NULL;
    if(this->first_node)
    {
        this->first_node->prev = node;
    }
    this->first_node = node;
    
}
/*--------------------------------------------------------- */
void
lub_heap_context_remove_node(lub_heap_context_t *this,
                             lub_heap_node_t    *node)
{
    lub_heap_node_t *next,*prev    = NULL;
    
    /* remove the node from the context list */
    next = lub_heap_node__get_next(node);
    prev = node->prev;

    if(NULL == prev)
    {
        this->first_node = next;
    }
    else
    {
        lub_heap_node__set_next(prev,next);
    }
    if(NULL != next)
    {
        next->prev = prev;
    }

    /* clear the pointers */
    lub_heap_node__set_next(node,NULL);
    node->prev = NULL;
    
    /* is this the last node in a context? */
    if(0 == this->allocs)
    {
        /* removing the last node deletes the context */
        lub_heap_context_delete(this);
    }
}
/*--------------------------------------------------------- */
void
lub_heap_context_show_frame(lub_heap_context_t * this,
                            int                  frame)
{
    if(frame >= 0)
    {
        long address = (long)this->key.backtrace[frame]; 
        if(address)
        {
            lub_heap_symShow(address);
        }
    }
    printf("\n");
}
/*--------------------------------------------------------- */
void
lub_heap_node_post(lub_heap_node_t *node)
{
    /* assume the worst */
    if(BOOL_TRUE == lub_heap_node__get_leaked(node))
    {
        /* this is a full leak */
        lub_heap_node__set_partial(node,BOOL_FALSE);
    }
}
/*--------------------------------------------------------- */
void
lub_heap_node_prep(lub_heap_node_t *node,
                   void            *arg)
{
    /* assume the worst */
    lub_heap_node__set_leaked(node,BOOL_TRUE);
    lub_heap_node__set_partial(node,BOOL_TRUE);
    lub_heap_node__set_scanned(node,BOOL_FALSE);
}
/*--------------------------------------------------------- */
void
lub_heap_node_scan(lub_heap_node_t *node,
                   void            *arg)
{
    /* only scan nodes which have references */
    if(   (BOOL_FALSE == lub_heap_node__get_leaked(node) )
       && (BOOL_FALSE == lub_heap_node__get_scanned(node)) )
    {
        lub_heap_node__set_scanned(node,BOOL_TRUE);
        lub_heap_scan_memory(lub_heap_node__get_ptr(node),
                             lub_heap_node__get_size(node));
    }
}
/*--------------------------------------------------------- */
static bool_t
lub_heap_context_prep(lub_heap_context_t *this,
                      void               *arg)
{
    /* start off by assuming the worst */
    this->partials         = this->leaks           = this->allocs;
    this->partial_bytes    = this->leaked_bytes    = this->alloc_bytes;
    this->partial_overhead = this->leaked_overhead = this->alloc_overhead;
    {
        lub_heap_leak_t *leak = lub_heap_leak_instance();
        /* initialised the global stats */
        leak->m_stats.allocs         += this->allocs;
        leak->m_stats.alloc_bytes    += this->alloc_bytes;
        leak->m_stats.alloc_overhead += this->alloc_overhead;
        lub_heap_leak_release(leak);
    }
    return BOOL_TRUE;
}
/*--------------------------------------------------------- */
static bool_t
lub_heap_context_post(lub_heap_context_t *this,
                      void               *arg)
{
    /* don't count full leaks as partials */
    this->partials         -= this->leaks;
    this->partial_bytes    -= this->leaked_bytes;
    this->partial_overhead -= this->leaked_overhead;
    
    /* post process the contained nodes */
    lub_heap_context_foreach_node(this,lub_heap_node_post);

    return BOOL_TRUE;
}
/*--------------------------------------------------------- */
static void
scan_segment(void    *ptr,
             unsigned index,
             size_t   size,
             void    *arg)
{
    lub_heap_segment_t *segment = ptr;
    const char         *memory  = (const char*)lub_heap_block_getfirst(segment);
    
    /* now scan the memory in this segment */
    printf(".");
    lub_heap_scan_memory(memory,size);
}
/*--------------------------------------------------------- */
/** 
 * This function scans all the nodes currently allocated in the
 * system for references to other allocated nodes.
 * First of all we mark all nodes as leaked, then scan all the nodes
 * for any references to other ones. If found those other ones 
 * are cleared from being leaked.
 * At the end of the process all nodes which are leaked then
 * update their context leak count.
 */
void
lub_heap_scan_all(void)
{
    lub_heap_t      *heap;
    lub_heap_leak_t *leak = lub_heap_leak_instance();
    

    /* clear the summary stats */
    memset(&leak->m_stats,0,sizeof(leak->m_stats));

    lub_heap_leak_release(leak);
    /* first of all prepare the contexts for scanning */
    lub_heap_foreach_context(lub_heap_context_prep,0);

    /* then prepare all the nodes (including those who have no context) */
    lub_heap_foreach_node(lub_heap_node_prep,0);
    
    printf("  Scanning memory");
    /* clear out the stacks in the system */
    lub_heap_clean_stacks();
    
    /* Scan the current stack */
    printf(".");
    lub_heap_scan_stack();
    
    /* Scan the BSS segment */
    printf(".");
    lub_heap_scan_bss();
    
    /* Scan the DATA segment */
    printf(".");
    lub_heap_scan_data();
    
    /* Scan the non-monitored blocks which are allocated in the system */
    leak = lub_heap_leak_instance();
    for(heap = leak->m_heap_list;
        heap;
        heap = heap->next) 
    {
        lub_heap_leak_release(leak);
        lub_heap_foreach_segment(heap,scan_segment,0);
        leak = lub_heap_leak_instance();
    }
    lub_heap_leak_release(leak);
    
    /* 
     * now scan the nodes NB. only referenced nodes will be scanned 
     * we loop until we stop scanning new nodes
     */
    leak = lub_heap_leak_instance();
    do
    {
        leak->m_stats.scanned = 0;
        /* scan each node */
        lub_heap_leak_release(leak);
        printf(".");
        lub_heap_foreach_node(lub_heap_node_scan,NULL);
        leak = lub_heap_leak_instance();
    } while(leak->m_stats.scanned);
        
    printf("done\n\n");

    /* post process each context and contained nodes */
    lub_heap_leak_release(leak);
    lub_heap_foreach_context(lub_heap_context_post,0);
    leak = lub_heap_leak_instance();
    
    
    lub_heap_leak_release(leak);
}
/*--------------------------------------------------------- */
void lub_heap_node_show(lub_heap_node_t *node)
{
    printf("%s%p[%"SIZE_FMT"] ",
           lub_heap_node__get_leaked(node) ? "*" : lub_heap_node__get_partial(node) ? "+" : "",
           lub_heap_node__get_ptr(node),
           lub_heap_node__get_size(node));
}
/*--------------------------------------------------------- */
static bool_t
lub_heap_context_match(lub_heap_context_t *this,
                       const char         *substring)
{
    bool_t result = BOOL_TRUE;
    if(substring)
    {
        int  i;
        long address;
        result = BOOL_FALSE;
        /* 
         * search the stacktrace for this context
         * to see whether it matches
         */
        for(i = 0;
            (address = (long)this->key.backtrace[i]);
            ++i)
        {
            if(lub_heap_symMatch(address,substring))
            {
                result = BOOL_TRUE;
                break;
            }
        }
    }
    return result;
}
/*--------------------------------------------------------- */
typedef struct
{
    lub_heap_show_e how;
    const char     *substring;
} context_show_arg_t;
/*--------------------------------------------------------- */
bool_t
lub_heap_context_show_fn(lub_heap_context_t *this,
                         void                *arg)
{
    bool_t              result = BOOL_FALSE;
    context_show_arg_t *show_arg = arg;
    
    lub_heap_leak_t *leak = lub_heap_leak_instance();
    /* add in the context details */
    ++leak->m_stats.contexts;
    leak->m_stats.allocs           += this->allocs;
    leak->m_stats.alloc_bytes      += this->alloc_bytes;
    leak->m_stats.alloc_overhead   += this->alloc_overhead;
    leak->m_stats.partials         += this->partials;
    leak->m_stats.partial_bytes    += this->partial_bytes;
    leak->m_stats.partial_overhead += this->partial_overhead;
    leak->m_stats.leaks            += this->leaks;
    leak->m_stats.leaked_bytes     += this->leaked_bytes;
    leak->m_stats.leaked_overhead  += this->leaked_overhead;
    lub_heap_leak_release(leak);
    
    
    if(lub_heap_context_match(this,show_arg->substring))
    {
        result = lub_heap_context_show(this,show_arg->how);
    }
    return result;
}
/*--------------------------------------------------------- */
bool_t
lub_heap_context_show(lub_heap_context_t *this,
                      lub_heap_show_e     how)
{
    long   frame       = lub_heap_frame_count-1;
    size_t ok_allocs   = this->allocs;
    size_t ok_bytes    = this->alloc_bytes;
    size_t ok_overhead = this->alloc_overhead;
    
    ok_allocs -= this->partials;
    ok_allocs -= this->leaks;

    ok_bytes  -= this->partial_bytes;
    ok_bytes  -= this->leaked_bytes;

    ok_overhead  -= this->partial_overhead;
    ok_overhead  -= this->leaked_overhead;

    switch(how)
    {
        case LUB_HEAP_SHOW_ALL:
        {
            /* show everything */
            break;
        }
        case LUB_HEAP_SHOW_PARTIALS:
        {
            if(0 < this->partials)
            {
                /* there's at least one partial in this context */
                break;
            }
            /*lint -e(616) fall through */
        }
        case LUB_HEAP_SHOW_LEAKS:
        {
            if(0 < this->leaks)
            {
                /* there's at least one leak in this context */
                break;
            }
            /*lint -e(616) fall through */
        }
        default:
        {
            /* nothing to be shown */
            return BOOL_FALSE;
        }
    }

    /* find the top of the stack trace */
    while((frame >= 0) && (0 == this->key.backtrace[frame]))
    {
        --frame;
    }
    printf("          +----------+----------+----------+----------+");
    lub_heap_context_show_frame(this,frame--);
    printf(      "%10p|    blocks|     bytes|   average|  overhead|",(void*)this);
    lub_heap_context_show_frame(this,frame--);
    printf("+---------+----------+----------+----------+----------+");
    lub_heap_context_show_frame(this,frame--);

    printf("|allocs   |%10"SIZE_FMT"|%10"SIZE_FMT"|%10"SIZE_FMT"|%10"SIZE_FMT"|",
            ok_allocs,
            ok_bytes,
            ok_allocs ? (ok_bytes / ok_allocs) : 0,
            ok_overhead);
    lub_heap_context_show_frame(this,frame--);

    if(this->partials)
    {
        printf("|partials |%10"SIZE_FMT"|%10"SIZE_FMT"|%10"SIZE_FMT"|%10"SIZE_FMT"|",
                this->partials,
                this->partial_bytes,
                this->partials ? (this->partial_bytes/this->partials) : 0,
                this->partial_overhead);
        lub_heap_context_show_frame(this,frame--);
    }
    if(this->leaks)
    {
        printf("|leaks    |%10"SIZE_FMT"|%10"SIZE_FMT"|%10"SIZE_FMT"|%10"SIZE_FMT"|",
                this->leaks,
                this->leaked_bytes,
                this->leaks ? (this->leaked_bytes/this->leaks) : 0,
                this->leaked_overhead);
        lub_heap_context_show_frame(this,frame--);
    }

    printf("+---------+----------+----------+----------+----------+");
    lub_heap_context_show_frame(this,frame--);

    while(frame >= 0)
    {
        printf("%55s","");
        lub_heap_context_show_frame(this,frame--);
    }

    printf("ALLOCATED BLOCKS: ");
    /* now iterate the allocated nodes */
    lub_heap_context_foreach_node(this,lub_heap_node_show);
    printf("\n\n");
    
    return BOOL_TRUE;
}
/*--------------------------------------------------------- */
void
lub_heap_context_clear(lub_heap_context_t *this)
{
    /* should only ever get cleared when all nodes have left the context */
    assert(0 == this->first_node);
    this->heap = 0;

    /* reset the counters */
    this->allocs        = 0;
    this->alloc_bytes   = 0;
    this->leaks         = 0;
    this->leaked_bytes  = 0;
    this->partials      = 0;
    this->partial_bytes = 0;
}
/*--------------------------------------------------------- */
void
lub_heap_show_leak_trees(void)
{
    lub_heap_leak_t *leak = lub_heap_leak_instance();
    printf("context_tree   : ");
    lub_bintree_dump(&leak->m_context_tree);
    printf("\n");
    printf("node_tree      : ");
    lub_bintree_dump(&leak->m_node_tree);
    printf("\n");
    printf("clear_node_tree: ");
    lub_bintree_dump(&leak->m_clear_node_tree);
    printf("\n");
    lub_heap_leak_release(leak);
}
/*--------------------------------------------------------- */
void
lub_heap_leak_scan(void)
{
    if(0 < lub_heap_frame_count)
    {
        /* check for memory leaks */
        if(!lub_heap_is_tainting())
        {
            printf("******************************************"
                   "*** Memory tainting is disabled so results\n"
                   "*** of scan may miss some real leaks!\n"
                   "******************************************\n\n");
        }
        lub_heap_scan_all();
    }
    else
    {
        printf("Leak detection currently disabled\n\n");
    }
}
/*--------------------------------------------------------- */
bool_t
lub_heap_leak_report(lub_heap_show_e how,
                     const char     *substring)
{
    bool_t result = BOOL_FALSE;
    
    if(0 < lub_heap_frame_count)
    {
        bool_t             bad_arg = BOOL_FALSE;
        lub_heap_leak_t   *leak    = lub_heap_leak_instance();
        context_show_arg_t show_arg;
        show_arg.how       = how;
        show_arg.substring = substring;
        
        /* zero these stats which will be accumulated from the contexts */
        leak->m_stats.contexts         = 0;
        leak->m_stats.allocs           = 0;
        leak->m_stats.alloc_bytes      = 0;
        leak->m_stats.alloc_overhead   = 0;
        leak->m_stats.partials         = 0;
        leak->m_stats.partial_bytes    = 0;
        leak->m_stats.partial_overhead = 0;
        leak->m_stats.leaks            = 0;
        leak->m_stats.leaked_bytes     = 0;
        leak->m_stats.leaked_overhead  = 0;
        lub_heap_leak_release(leak);
        
        switch(how)
        {
            case LUB_HEAP_SHOW_LEAKS:
            case LUB_HEAP_SHOW_PARTIALS:
            case LUB_HEAP_SHOW_ALL:
            {
                result = lub_heap_foreach_context(lub_heap_context_show_fn,(void*)&show_arg);
                break;
            }
            default:
            {
                printf("Invalid argument: 0=leaks, 1=partials and leaks, 2=all allocations\n\n");
                bad_arg = BOOL_TRUE;
                break;
            }
        }
        if(BOOL_FALSE == bad_arg)
        {
            leak = lub_heap_leak_instance();
            if(leak->m_stats.leaks)
            {
                printf("  '*' = leaked memory\n");
            }
            if(leak->m_stats.partials)
            {
                printf("  '+' = partially leaked memory\n");
            }
            lub_heap_leak_release(leak);
            lub_heap_show_summary();
            printf("  %lu stack frames held for each allocation.\n\n",lub_heap_frame_count);
        }
    }
    else
    {
        printf("Leak detection currently disabled\n\n");
    }
    return result;
}
/*--------------------------------------------------------- */

/*--------------------------------------------------------- 
 * PUBLIC META FUNCTIONS
 *--------------------------------------------------------- */
void
lub_heap__set_framecount(unsigned frames)
{
    static bool_t    clear_tainting_on_disable = BOOL_FALSE;
    lub_heap_leak_t *leak;
    
    if(frames == lub_heap_frame_count)
        return;
    
    /* iterate around clearing all the nodes in the node_tree */
    lub_heap_foreach_node(lub_heap_node_clear,0);
    
    leak = lub_heap_leak_instance();
    if(frames)
    {
        static bool_t registered = BOOL_FALSE;
        if(frames <= MAX_BACKTRACE)
        {
            /* change the number of frames held */
            lub_heap_frame_count = frames;
        }
        else
        {
            fprintf(stderr,"--- leak-detection frame count set to a maximum of %d\n",MAX_BACKTRACE);
            lub_heap_frame_count = MAX_BACKTRACE;
        }
        if(!lub_heap_is_tainting())
        {
            /* we need to taint memory to ensure old pointers don't mask leaks */
            clear_tainting_on_disable = BOOL_TRUE;
            lub_heap_taint(BOOL_TRUE);
        }
        if(BOOL_FALSE == registered)
        {
            registered = BOOL_TRUE;
            /* 
             * set up the context pool
             */
            lub_dblockpool_init(&leak->m_context_pool,
                                sizeof(lub_heap_context_t),
                                CONTEXT_CHUNK_SIZE,
                                CONTEXT_MAX_CHUNKS);
        }
    }
    else
    {
        /* switch off leak detection */
        lub_heap_frame_count = 0;
        if(clear_tainting_on_disable)
        {
            lub_heap_taint(BOOL_FALSE);
            clear_tainting_on_disable = BOOL_FALSE;
        }
    }
    lub_heap_leak_release(leak);   
}
/*--------------------------------------------------------- */
unsigned
    lub_heap__get_framecount(void)
{
    return lub_heap_frame_count;
}
/*--------------------------------------------------------- */
size_t
    lub_heap_context__get_instanceSize(void)
{
    return (sizeof(lub_heap_context_t));
}
/*--------------------------------------------------------- */
lub_heap_context_t *
lub_heap_context_find(const stackframe_t *stack)
{
    lub_heap_context_t    *result;
    lub_heap_leak_t       *leak = lub_heap_leak_instance();
    lub_heap_context_key_t key  = *stack;

    result = lub_bintree_find(&leak->m_context_tree,&key);

    lub_heap_leak_release(leak);
    return result;
}
/*--------------------------------------------------------- */
