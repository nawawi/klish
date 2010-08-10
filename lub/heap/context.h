/* 
 * context.h 
 */
/**********************************************************
 * CONTEXT CLASS DEFINITIONS
 ********************************************************** */
#define MAX_BACKTRACE (32)

typedef struct _lub_heap_stackframe lub_heap_context_key_t;
typedef void (function_t)(void);

struct _lub_heap_stackframe
{
    function_t * backtrace[MAX_BACKTRACE];
};

typedef struct _lub_heap_leak_stats lub_heap_leak_stats_t;
struct _lub_heap_leak_stats
{
    size_t contexts;
    size_t allocs;
    size_t alloc_bytes;
    size_t alloc_overhead;
    size_t leaks;
    size_t leaked_bytes;
    size_t leaked_overhead;
    size_t partials;
    size_t partial_bytes;
    size_t partial_overhead;
    size_t scanned;
};

struct _lub_heap_context
{
    /**
     * The heap to which this context belongs
     */
    lub_heap_t *heap;
    /**
     * This is needed to maintain a tree of contexts
     */
    lub_bintree_node_t bt_node;
    /**
     * The first node in this context
     */
     lub_heap_node_t *first_node;
    /**
     * current number of allocations made from this context
     */
    size_t allocs;
    /**
     * current number of bytes allocated from this context
     */
    size_t alloc_bytes;
    /**
     * current overhead in bytes allocated from this context
     */
    size_t alloc_overhead;
    /**
     * count of leaked allocations made from this context
     */
    size_t leaks;
    /**
     * number of leaked bytes from this context
     */
    size_t leaked_bytes;
    /**
     * current overhead in bytes leaked from this context
     */
    size_t leaked_overhead;
    /**
     * count of partially leaked allocations made from this context
     */
    size_t partials;
    /**
     * number of partially leaked bytes from this context
     */
    size_t partial_bytes;
    /**
     * current overhead in bytes partially leaked from this context
     */
    size_t partial_overhead;
    /**
     * a record of the memory partition associated with this context
     */
    lub_heap_context_key_t key;
};

extern unsigned long lub_heap_frame_count;

typedef struct _lub_heap_leak lub_heap_leak_t;
struct _lub_heap_leak
{
    lub_bintree_t         m_context_tree;
    lub_bintree_t         m_node_tree;
    lub_bintree_t         m_clear_node_tree;
    lub_bintree_t         m_segment_tree;
    lub_heap_leak_stats_t m_stats;
    lub_dblockpool_t      m_context_pool;
    lub_heap_t           *m_heap_list;
};

extern lub_heap_leak_t *
    lub_heap_leak_instance(void);

extern void
    lub_heap_leak_release(lub_heap_leak_t *instance);

extern bool_t
    lub_heap_leak_query_node_tree(void);

extern bool_t
    lub_heap_leak_query_clear_node_tree(void);
/*---------------------------------------------------------
 * PUBLIC META ATTRIBUTES
 *--------------------------------------------------------- */
/**
 * Obtains the number of bytes required to allocate a context instance.
 *
 * By default this is set to four.
 */
size_t
    lub_heap_context__get_instanceSize(void);

extern lub_bintree_compare_fn lub_heap_context_compare;
extern lub_bintree_getkey_fn  lub_heap_context_getkey;

/*---------------------------------------------------------
 * PUBLIC METHODS
 *--------------------------------------------------------- */
/**
 * This function finds or creates a context based on the
 * provided leak detector and stack specification.
 * 
 * A context is defined by a pre-configured number of stack frames
 * (see lub_heap_context__set_frameCount()) 
 * All memory allocations which occur with 
 * the same stack frames will be collected together in the same context.
 *
 * \return Pointer to a context object or NULL if one cannot be found or allocated.
 */ 
 const lub_heap_context_t *
    lub_heap_context_find_or_create(
        /** 
         * The heap in question
         */
        lub_heap_t *instance,
        /** 
         * The memory allocation function must allocate a
         * local variable, on it's stack, which is passed in 
         * to this call as a means of getting access to the current
         * thread's call stack.
         */
        const stackframe_t * stack
    );
/**
 * This function finds a context based on the
 * provided leak detector and stack specification.
 * 
 * A context is defined by a pre-configured number of stack frames
 * (see lub_heap_context__set_frameCount()) 
 * All memory allocations which occur with 
 * the same stack frames will be collected together in the same context.
 *
 * \return Pointer to a context object or NULL if one cannot be found.
 */ 
lub_heap_context_t *
    lub_heap_context_find(
        /** 
         * The memory allocation function must allocate a
         * local variable, on it's stack, which is passed in 
         * to this call as a means of getting access to the current
         * thread's call stack.
         */
        const stackframe_t * stack
    );
    
/*---------------------------------------------------------
 * PUBLIC METHODS
 *--------------------------------------------------------- */
/**
 * This function initialises an instance of a context.
 */
void
    lub_heap_context_init(
        /** 
         * Context instance on which to operate
         */
        lub_heap_context_t *instance,
        /** 
         * heap with with to associate this context
         */
        lub_heap_t *heap,
        /**
         * The memory allocation function must allocate a
         * local variable, on it's stack, which is passed in 
         * to this call as a means of getting access to the current
         * thread's call stack.
         */
        const stackframe_t * stack
    );
/**
 * This function finalises an instance of a context.
 * 
 * \return the memory partition from which this context was allocated.
 */
void
    lub_heap_context_fini(
        /** 
         * Context instance on which to operate
         */
        lub_heap_context_t *instance
    );
/**
 * This function deletes an instance of a context
 * 
 * \return BOOL_TRUE if deletion was sucessful
 */
bool_t
    lub_heap_context_delete(
        /** 
         * Context instance on which to operate
         */
        lub_heap_context_t *instance
    );
/**
 * This function adds the specified node to the context tree.
 *
 * \return BOOL_TRUE  if the node was added to the tree,
 *          BOOL_FALSE if node was already in the tree.
 */
bool_t
    lub_heap_context_add_node(
        /** 
         * Context instance on which to operate
         */
        lub_heap_context_t *instance,
        /**
         * The node to add to this context 
         */
         lub_heap_node_t *node,
        /**
         * The size in bytes of the allocation 
         */
         size_t size
    );
/**
 * This function removes the specified node from the context tree.
 *
 * \return BOOL_TRUE  if the node was found and removed,
 *          BOOL_FALSE if the node was not in the tree
 */ 
void
    lub_heap_context_remove_node(
        /** 
         * Context instance on which to operate
         */
        lub_heap_context_t *instance,
        /**
         * The node to add to this context 
         */
         lub_heap_node_t *node
    );
/**
 * This function dumps details of the specified context to stdout
 *
 * /return
 * - BOOL_TRUE if any details were dumped.
 * - BOOL_FALSE if no details were dumped.
 */ 
bool_t 
    lub_heap_context_show(
        /** 
         * Context instance on which to operate
         */
        lub_heap_context_t *instance,
        /** 
         * How to dump the information
         */
        lub_heap_show_e how
    );
/**
 * This function "clears" the currently allocated blocks so that lub_heap_context_dump
 * will not show them
 */ 
void 
    lub_heap_context_clear(
        /** 
         * Context instance on which to operate
         */
        lub_heap_context_t *instance
    );
/**
 * This places a node into the context's tree
 */
 void
    lub_heap_context_insert_node(
        lub_heap_context_t * instance,
        lub_heap_node_t * node
    );
/*---------------------------------------------------------
 * PUBLIC ATTRIBUTES
 *--------------------------------------------------------- */
/**
 * Identify whether this is a valid address within the heap
 */
 bool_t
    leaks_is_valid_heap_address(lub_heap_t *instance,
                                const void *ptr);

void
    lub_heap_leak_mutex_lock(void);
void
    lub_heap_leak_mutex_unlock(void);
