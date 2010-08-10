/* node.h */

/**********************************************************
 * NODE CLASS DEFINTIONS
 ********************************************************** */

#define LEAKED_MASK 0x00000001
#define PARTIAL_MASK 0x00000002

#define SCANNED_MASK 0x00000001

struct _lub_heap_node
{
    lub_heap_context_t *_context; /* bottom two bits used for leaked, partial flags */
    lub_heap_node_t    *_next;    /* linked list of nodes per context (bottom bit used for scanned mask) */
    lub_heap_node_t    *prev;
    lub_bintree_node_t  bt_node;
};

typedef struct _lub_heap_node_key lub_heap_node_key_t;
struct _lub_heap_node_key
{
    const lub_heap_node_t *node;
};

/*---------------------------------------------------------
 * PUBLIC META DATA
 *--------------------------------------------------------- */
/*
 * Get the size (in bytes) of an instance of a node object
 */
extern size_t
    lub_heap_node__get_instanceSize(void);

/*---------------------------------------------------------
 * PUBLIC META FUNCTIONS
 *--------------------------------------------------------- */
extern void
    lub_heap_node_clear(lub_heap_node_t *instance,
                        void            *arg);

extern void
    lub_heap_foreach_node(void (*fn)(lub_heap_node_t *, void*),void *arg);

extern void 
    lub_heap_scan_memory(const void  *mem,
                         size_t       size);
/*
 * Given a pointer to the start of a data area of a node obtain the
 * node reference. Returns NULL if the node is not valid. 
 */
extern lub_heap_node_t *
    lub_heap_node_from_start_of_block_ptr(char *ptr);

extern void
    lub_heap_node_initialise(void);

/*---------------------------------------------------------
 * PUBLIC METHODS
 *--------------------------------------------------------- */
/*
 * Given a node return a pointer to the data area allocated.
 */
extern char *
    lub_heap_node__get_ptr(
        const lub_heap_node_t *instance
    );
/*
 * Given a node return the size of memory that it encapsulates.
 */
extern size_t
    lub_heap_node__get_size(
        const lub_heap_node_t *instance
    );
/*
 * Given a node return the size of memory overhead that it encapsulates.
 */
extern size_t
    lub_heap_node__get_overhead(
        const lub_heap_node_t *instance
    );
/*
 * Get a node to remove itself from it's allocation tree
 */
extern void
    lub_heap_node_fini(
        lub_heap_node_t *instance
    );

extern void
    lub_heap_node_init(
        lub_heap_node_t * instance,
        lub_heap_context_t * context
    );

extern lub_bintree_compare_fn lub_heap_node_compare;
extern lub_bintree_getkey_fn  lub_heap_node_getkey;
    
extern bool_t
    lub_heap_node__get_leaked(
        const lub_heap_node_t *instance
    );

extern bool_t
    lub_heap_node__get_partial(
        const lub_heap_node_t *instance
    );
    
extern bool_t
    lub_heap_node__get_scanned(
        const lub_heap_node_t *instance
    );

extern void
    lub_heap_node__set_leaked(
        lub_heap_node_t *instance,
        bool_t        value
    );

extern void
    lub_heap_node__set_partial(
        lub_heap_node_t *instance,
        bool_t        value
    );

extern void
    lub_heap_node__set_scanned(
        lub_heap_node_t *instance,
        bool_t        value
    );

extern lub_heap_context_t *
    lub_heap_node__get_context(
        const lub_heap_node_t *instance
    );

extern void
    lub_heap_node__set_context(
        lub_heap_node_t    *instance,
        lub_heap_context_t *context
    );
    
extern lub_heap_node_t *
    lub_heap_node__get_next(
        const lub_heap_node_t *instance
    );

extern void
    lub_heap_node__set_next(
        lub_heap_node_t *instance,
        lub_heap_node_t *context
    );
