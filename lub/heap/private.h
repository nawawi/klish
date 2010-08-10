/*
 * private.h
 */
#include "lub/heap.h"
#include "lub/bintree.h"
#include "lub/types.h"
#include "lub/dblockpool.h"
#include "lub/size_fmt.h"

/**********************************************************
 * PRIVATE TYPES
 ********************************************************** */
typedef struct _lub_heap_node    lub_heap_node_t;    
typedef struct _lub_heap_context lub_heap_context_t;    
typedef struct _lub_heap_cache   lub_heap_cache_t;    

typedef enum {
    LUB_HEAP_TAINT_INITIAL = 0xBB,
    LUB_HEAP_TAINT_FREE    = 0xFF,
    LUB_HEAP_TAINT_ALLOC   = 0xAA
} lub_heap_taint_t;

typedef struct _lub_heap_stackframe stackframe_t;

/**********************************************************
 * PRIVATE TYPES
 ********************************************************** */
/* type to indicate the number of long words in a block */
typedef unsigned long                 words_t;
typedef struct lub_heap_tag_s         lub_heap_tag_t;
typedef struct lub_heap_segment_s     lub_heap_segment_t;
typedef union  lub_heap_block_u       lub_heap_block_t;
typedef struct lub_heap_alloc_block_s lub_heap_alloc_block_t;

/*
 * a special type which hold both a size and block type
 */
struct lub_heap_tag_s
{
    unsigned free    : 1; /* indicates whether free/alloc block */
    unsigned segment : 1; /* indicates boundary of segment */
    unsigned words   : 30; /* number of 4 byte words in block */
};

typedef struct lub_heap_key_s lub_heap_key_t;
struct lub_heap_key_s
{
    words_t                      words;
    const lub_heap_free_block_t *block;
};
/*
 * A specialisation of a generic block to hold free block information
 */
struct lub_heap_free_block_s
{
    lub_heap_tag_t tag;
    /*
     * node to hold this block in the size based binary tree 
     */
    lub_bintree_node_t bt_node;
    /* space holder for the trailing tag */
    lub_heap_tag_t _tail;
};

/*
 * A specialisation of a generic block to hold free block information
 */
struct lub_heap_alloc_block_s
{
    lub_heap_tag_t tag;
    /* space holder for trailing tag */
    lub_heap_tag_t memory[1];
};

/* a generic block instance */
union lub_heap_block_u
{
    lub_heap_alloc_block_t alloc;
    lub_heap_free_block_t  free;
};

/* 
 * This is the control block for a segment
 */
struct lub_heap_segment_s
{
    lub_heap_segment_t *next;
    words_t             words;
    lub_bintree_node_t  bt_node;
};

/*
 * This is the control block assigned to represent the heap instance
 */
struct lub_heap_s
{
    /*
     * A cache control block if one exists
     */
    lub_heap_cache_t *cache;
    /*
     * A binary tree of free blocks 
     */
    lub_bintree_t free_tree;
    /*
     * This is used to supress leak tracking
     */
    unsigned long suppress;
    /* 
     * statistics for this heap 
     */
    lub_heap_stats_t stats;
    /*
     * reference to next heap in the system
     */
    struct lub_heap_s *next;
    /* 
     * reference to next segment (this must be last in the structure)
     */
    lub_heap_segment_t first_segment;
};

/*---------------------------------------------------------
 * PRIVATE META DATA
 *--------------------------------------------------------- */
/*
 * a pointer to the start of data segment
 */
extern char *lub_heap_data_start;
/*
 * a pointer to the end of data segment
 */
extern char *lub_heap_data_end;

/*---------------------------------------------------------
 * PRIVATE META FUNCTIONS
 *--------------------------------------------------------- */
/*
 * This operation sends to STDOUT the textual representation
 * of the provided address in memory.
 */
extern void
    lub_heap_symShow(
        /*
         * The address of the function name to resolve
         */
        unsigned address
    );
/*
 * This operation indicates whether the function represented by the
 * specified address contains the specified string.
 */
extern bool_t
    lub_heap_symMatch(
        /*
         * The address of the function name to resolve
         */
        unsigned address,
        /*
         * The substring to match against
         */
        const char *substring
    );
/*
 * This operation fills out the current stackframe
 */
extern void
    lub_heap__get_stackframe(stackframe_t *stack,unsigned max);
/*
 * A platform implementation should scan the BSS section
 * for any memory references.
 */
extern void
    lub_heap_scan_bss(void);
/*
 * A platform implementation should scan the DATA section
 * for any memory references.
 */
extern void
    lub_heap_scan_data(void);
/*
 * If possible a platform implementation should scan the current
 * stack for any memory references.
 * NB. if a platform happens to have allocated it stack from the 
 * BSS or DATA section then this doesn't need to do anything.
 */
extern void
    lub_heap_scan_stack(void);
/*
 * This function fills out the specified memory with tainted
 * bytes if tainting is enabled for the system.
 */
extern void 
    lub_heap_taint_memory(
        /*
         * a pointer to the start of the memory to taint
         */
        char            *ptr,
        /*
         * the type of tainting to perform
         */
        lub_heap_taint_t type,
        /*
         * the number of bytes to taint
         */
        size_t           size 
    );
/*
 * If possible a platform implementation should
 * take this oportunity to write the value 0xCC 
 * to all the stacks between the high water mark 
 * and the current stack pointer.
 * This prevents any pointer droppings from hiding
 * memory leaks.
 */
extern void
    lub_heap_clean_stacks(void);

/*---------------------------------------------------------
 * PRIVATE METHODS
 *--------------------------------------------------------- */
/*
 * This operation is the raw interface for allocating/releasing memory
 * to/from the dynamic heap. Memory allocated by this call will not be
 * monitored by the leak detection code.
 *
 * It changes the size of the object referenced by a passed in 
 * pointer to "size". The contents will be unchanged up to the minimum of the old 
 * and new sizes. If the new size is larger, the new space is uninitialised.
 *
 * \pre
 * - The heap needs to have been created with an initial memory segment.
 * - If "*ptr" contains a non-NULL value then this MUST have 
 *   been allocated using this operation, from the same heap instance.
 *
 * \return 
 * - the status of the operation.
 *
 * \post
 * - The client takes responsiblity for releasing any allocated memory when they
 *   are finished with it.
 * - If *ptr contains a non-NULL value, then after a succesfull call, the 
 *   initial memory referenced by it may have been released for reuse,
 *   and the pointer modified to reference some new memory.
 * - *ptr may contain NULL in which case no memory will be released back to the
 *   heap for reuse, and the pointer is filled out with the allocated memory.
 * - (size == 0) No new memory will be allocated, *ptr will be set to NULL,
 *   and any original memory referenced by it will have been released.
 */
extern lub_heap_status_t
    lub_heap_raw_realloc(
        /*
         * The heap instance.
         */
        lub_heap_t *instance,
        /*
         * A reference to the client pointer which was previously handed out.
         */
        char **ptr,
        /*
         * The size of the requested memory
         */
        size_t size,
        /*
         * The alignment of the requested memory
         */
        lub_heap_align_t alignment);
/*
 * This is called before an actual realloc operation runs.
 * It provides the opportunity for the leak detector to release
 * any previous details and to modify the size and pointer to account
 * for its internal overheads.
 *
 * \return 
 * The number of bytes which should be allocated by the realloc call.
 *
 * \post
 * - the 'node' currently held in the database will be removed, potentially 
 *   deleting the 'context' in the process.
 * - the leak_detector will note the fact that it is wrapping a realloc
 *   call and any further calls which are invoked during the process will 
 *   NOT be monitored.
 */
extern void
    lub_heap_pre_realloc(
        /*
         * The heap instance.
         */
        lub_heap_t *instance,
        /*
         * A reference to the client pointer which was previously handed out.
         */
        char **ptr,
        /*
         * A reference to the size of the requested memory
         */
        size_t *size);
/*
 * This is called after a realloc has been done, just before the results
 * are returned to the client. 
 * It provides the opportunity for the leak detector to store details 
 * on the allocation and track it.
 *
 * \return 
 * The pointer which should be returned to the client. This will skip any
 * housekeeping details which will have been added to the start of the
 * block.
 *
 * \post
 * - the 'node' currently will be added into the database, creating a
 *   context if appropriate.
 * - the leak_detector will note the fact that the wrapping of a realloc()
 *   call has completed and will start to monitor further calls again.
 */
extern void
    lub_heap_post_realloc(
        /*
         * The heap instance
         */
        lub_heap_t *instance,
        /*
         * A reference to the client pointer which has been allocated.
         */
        char **ptr
    );
/*
 * This function clears all the current leak information from the heap
 *
 * It is also used to define the number of stack frames which are 
 * used to specify a context.
 *
 */
void
    lub_heap_clearAll(
        /*
         * The heap instance 
         */
         lub_heap_t *instance,
        /*
         * The number of stack frames to account for when evaluating
         * a context. A value of zero means "don't change"
         */
        unsigned frames
    );
/*
 * This allows the tracking of allocation failures.
 */
 void
    lub_heap_alloc_failed(lub_heap_context_t *instance);

/*
 * This allows the tracking of free failures.
 */
 void
    lub_heap_free_failed(void);

/*
 * This method slices the specified number of words off the
 * bottom of the specified free block.
 * The original block is then shrunk appropriately and the 
 * address of the new free block updated accordingly.
 * The result returned is a pointer to the requested memory.
 * If the slice cannot be taken then NULL is returned and the
 * free block is unaltered.
 *
 * It is assumed that the free block has been removed from the
 * tree prior to calling this method.
 */
extern void *
    lub_heap_slice_from_bottom(lub_heap_t             *instance,
                               lub_heap_free_block_t **ptr_block,
                               words_t                *words,
                               bool_t                  seg_start);
/* 
 * This method slices the specified number of words off the
 * top of the current free block.
 * The block is then shrunk appropriately and the assigned memory
 * is returned. The client's free block pointer is updated accordingly
 * e.g. if the request absorbs all of the free_block the client's 
 * pointer is set to NULL.
 * If there is insufficient space to slice this free block then
 * NULL is returned and the free block is unaltered.
 */
extern void *
    lub_heap_slice_from_top(lub_heap_t             *instance,
                            lub_heap_free_block_t **ptr_block,
                            words_t                *words,
                            bool_t                  seg_end);
/*
 * This method initialises a free block from some specified memory.
 */
extern void
    lub_heap_init_free_block(lub_heap_t *instance,
                             void       *ptr,
                             size_t      size,
                             bool_t      seg_start,
                             bool_t      seg_end);
/* 
 * This method takes an existing allocated block and extends it by 
 * trying to take memory from a free block immediately 
 * above it
 */
extern bool_t
    lub_heap_extend_upwards(lub_heap_t        *instance,
                            lub_heap_block_t **block,
                            words_t            words);
/* 
 * This method takes an existing allocated block and extends it by 
 * trying to take memory from a free block immediately 
 * below it
 */
extern bool_t
    lub_heap_extend_downwards(lub_heap_t        *instance,
                              lub_heap_block_t **block,
                              words_t            words);
/* 
 * This method takes an existing allocated block and extends it by 
 * trying to take memory from free blocks immediately 
 * above and below it.
 */
extern bool_t
    lub_heap_extend_both_ways(lub_heap_t        *instance,
                              lub_heap_block_t **block,
                              words_t            words);
/*
 * This method creates a new alloc block from the specified heap
 */
extern void *
    lub_heap_new_alloc_block(lub_heap_t *instance,
                             words_t     words);
extern void
    lub_heap_graft_to_top(lub_heap_t       *instance,
                          lub_heap_block_t *free_block,
                          void             *ptr,
                          words_t           words,
                          bool_t            seg_end,
                          bool_t            other_free_block);
extern void
    lub_heap_graft_to_bottom(lub_heap_t       *instance,
                             lub_heap_block_t *free_block,
                             void             *ptr,
                             words_t           words,
                             bool_t            seg_start,
                             bool_t            other_free_block);
extern lub_heap_status_t
    lub_heap_merge_with_next(lub_heap_t       *instance,
                             lub_heap_block_t *block);
extern lub_heap_status_t
    lub_heap_merge_with_previous(lub_heap_t       *instance,
                                 lub_heap_block_t *block);
extern lub_heap_status_t 
    lub_heap_new_free_block(lub_heap_t       *instance,
                            lub_heap_block_t *block);
/* 
 * Given a block reference find the trailing tag
 */
extern lub_heap_tag_t *
    lub_heap_block__get_tail(lub_heap_block_t *block);

/*
 * Find the first block reference in the specified segment
 */
lub_heap_block_t *
    lub_heap_block_getfirst(const lub_heap_segment_t *seg);
/* 
 * Given a block reference find the next one
 */
extern lub_heap_block_t *
    lub_heap_block_getnext(lub_heap_block_t *block);
/* 
 * Given a block reference find the previous one
 */
extern lub_heap_block_t *
    lub_heap_block_getprevious(lub_heap_block_t *block);
/* 
 * This checks the integrity of the current block
 * If the header and trailer values are different then
 * some form of memory corruption must have occured.
 */
extern bool_t
    lub_heap_block_check(lub_heap_block_t *block);

extern void
    lub_heap_block_getkey(const void        *clientnode,
                          lub_bintree_key_t *key);
extern void
    lub_heap_scan_all(void);

extern bool_t
    lub_heap_check_memory(lub_heap_t *instance);

extern lub_heap_block_t *
    lub_heap_block_from_ptr(char *ptr);

extern const lub_heap_segment_t *
    lub_heap_segment_findnext(const void *address);

extern void
    lub_heap_align_block(lub_heap_t      *this,
                         lub_heap_align_t alignment,
                         char           **ptr,
                         size_t          *size);
lub_heap_status_t
    lub_heap_cache_realloc(lub_heap_t      *this,
                           char           **ptr,
                           size_t           requested_size);
void
    lub_heap_cache_show(lub_heap_cache_t *this);
