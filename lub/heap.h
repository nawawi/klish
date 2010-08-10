/*
 * heap.h
 */
/**
\ingroup lub
\defgroup lub_heap heap
@{

\brief This is a generic heap manager which incorporates a memory leak detection system 
which can monitor and report the dynamic memory which is in use for each heap.

A client creates an instance of heap; providing it with memory segments
to manage. Subsequent operations can then be invoked to obtain memory
from those registered segments.

\section static_allocs Static allocation
Static memory allocations are those which are for the lifetime of the
heap from which they are allocated. Because they do not need to be freed
there is zero overhead required for such blocks; they can be exactly butted 
up against each other in memory. There is also zero fragmentation as they 
are never freed.

\section dynamic_allocs Dynamic allocation
Dynamic memory allocations have a lifetime less than that of the heap 
from which they are allocated. In order to manage the reuse of this blocks
once they are released, there will be a slight "housekeeping" overhead
associated with each block. They are also suceptible to fragmentation, which 
can occur when the lifetimes of blocks differ from one another.

\section leak_detection Leak Detection
It monitors the dynamic allocation which are performed and maintains 
statistics to help identify and isolation memory leaks.

It detects two types of leak by scanning the BSS and DATA segments and identifying 
any nodes referenced from there. Then each of these referenced nodes is scanned to 
look for further references.

\subsection full_leak Full Leak
there is no reference to a block of memory or to any memory within that block in the system.

\subsection partial_leak Partial Leak
there is no reference to the start of a block of memory. NB. there
may be circumstances where this is not a real leak, e.g. memory allocation
systems typically hand out references just beyond a control header to their
clients. However there may also be instances where this is a real leak 
and someone just happens to have a reference to some contained data.

\section tainting  Memory Tainting
Memory is deliberately dirtied in the following ways:

-   Initial heap space - 0xBBBBBBBB
-   Allocated memory   - 0xAAAAAAAA
-   Freed memory       - 0xFFFFFFFF
-   Dead Stack memory  - 0xCCCCCCCC (done before leak scan)

\section usage Utility functions
Currently the following utility functions are available:

- leakScan [0|1|2]      - provides a dump of the currently allocated blocks of 
                          memory in the system pool.
                          argument values have the following meanings:
                          - 0 - display stats for just memory leaks.
                          - 1 - display stats for memory leaks and partial leaks.
                          - 2 - display stats for all currently allocation memory 
                          blocks.

- leakEnable [framecount] - enables leak detection and causes the current statistics to be 
                          cleared out.
                          Even if there are memory blocks in use, this
                          command will cause future "leakShow" invocations
                          to behave as if monitoring started from this new
                          point in time.
                          The optional 'framecount' argument can be used 
                          to change the number of levels of backtrace
                          recorded. By default this number is 16. The
                          number of contexts stored will be affected by
                          this number. If the value is small then a
                          limited number of contexts will exist and the 
                          memory overhead of monitoring will be reduced. 
                          If the number is large (can support a maximum 
                          of 16 levels) then the granularity of the
                          contexts will be finer i.e. more of them, but 
                          the memory overhead in monitoring will be 
                          increased.
                          With no specified framecount the maximum will be assumed.

- leakDisable             - Disabled the leak detection.
    
\section implementation Implementation
Static and dynamic blocks are allocated from opposite ends of a memory
segment. As static blocks are allocated the end of the last 
"free block" migrates downwards in memory. Dynamic blocks are allocated 
from the start of the appropriately sized free block, hence leaving
space for static allocations at the end of the heap memory.

The heap implements a "best fit" model, for allocation of dynamic memory.
This means that free blocks are maintained in size order and hence
the most appropriate one can be used to satisfy client requests.
This minimises fragmentation of the dynamically allocated memory.

The free blocks are held in a binary tree (using \ref lub_bintree) which provide
fast searching for the appropriate block.

\author  Graeme McKerrell
\date    Created On      : Wed Dec 14 10:20:00 2005
\version UNTESTED
*/
#ifndef _lub_heap_h
#define _lub_heap_h
#include <stddef.h>

#include "lub/types.h"
#include "lub/c_decl.h"
_BEGIN_C_DECL

/**
 * This type is used to reference an instance of a heap.
 */
typedef struct lub_heap_s lub_heap_t;

/**
 * This type is used to reference an instance of a free block
 */
typedef struct lub_heap_free_block_s lub_heap_free_block_t;

/**
 * This type defines the statistics available for each heap.
 */
typedef struct lub_heap_stats_s lub_heap_stats_t;
struct lub_heap_stats_s
{
    /*----------------------------------------------------- */
    /**
     * Number of segments comprising this heap.
     */
    size_t segs;
    /**
     * Number of bytes available in all the segments.
     */
    size_t segs_bytes;
    /**
     * Number of bytes used in housekeeping overheads
     */
    size_t segs_overhead;
    /*----------------------------------------------------- */
    /**
     * Number of free blocks. This is indication 
     * the fragmentation state of a heap.
     */
    size_t free_blocks;
    /**
     * Number of bytes available in a heap.
     */
    size_t free_bytes;
    /**
     * Number of bytes used in housekeeping overheads
     */
    size_t free_overhead;
    /*----------------------------------------------------- */
    /**
     * Number of dynamically allocated blocks currently
     * held by clients of a heap.
     */
    size_t alloc_blocks;
    /**
     * Number of dynamically allocated bytes currently
     * held by clients of a heap.
     */
    size_t alloc_bytes;
    /**
     * Number of bytes used in housekeeping overheads
     */
    size_t alloc_overhead;
    /*----------------------------------------------------- */
    /**
     * Cumulative number of dynamically allocated blocks
     * given to clients of a heap.
     */
    size_t alloc_total_blocks;
    /**
     * Cumulative number of dynamically allocated bytes
     * given to clients of a heap.
     */
    size_t alloc_total_bytes;
    /*----------------------------------------------------- */
    /**
     * Number of dynamically allocated blocks
     * given to clients when the memory usage was at it's highest.
     */
    size_t alloc_hightide_blocks;
    /**
     * Number of dynamically allocated bytes
     * given to clients of a heap when the memory usage was at it's
     * highest
     */
    size_t alloc_hightide_bytes;
    /**
     * Number of bytes of overhead in use when the memory usage 
     * was at it's highest
     */
    size_t alloc_hightide_overhead;
    /**
     * Number of free blocks when the memory usage was at it's
     * highest
     */
    size_t free_hightide_blocks;
    /**
     * Number of free bytes when the memory usage was at it's highest.
     */
    size_t free_hightide_bytes;
   /**
     * Number of housekeeping overhead bytes when the memory 
     * usage was at it's highest.
     */
    size_t free_hightide_overhead;
    /*----------------------------------------------------- */
    /**
     * Number of statically allocated blocks currently
     * held by clients of a heap.
     */
    size_t static_blocks;
    /**
     * Number of statically allocated bytes currently
     * held by clients of a heap.
     */
    size_t static_bytes;    
    /**
     * Number of dynamically allocated bytes currently
     * held by clients of a heap.
     */
    size_t static_overhead;
    /*----------------------------------------------------- */
};
/**
 * This type is used to indicate the result of a dynamic
 * memory allocation
 */
typedef enum
{
    /**
     * The allocation was successful
     */
    LUB_HEAP_OK,
    /**
     * There was insufficient resource to satisfy the request
     */
    LUB_HEAP_FAILED,
    /**
     * An attempt has been made to release an already freed block
     * of memory.
     */
    LUB_HEAP_DOUBLE_FREE,
    /**
     * A memory corruption has been detected. e.g. someone writing
     * beyond the bounds of an allocated block of memory.
     */
    LUB_HEAP_CORRUPTED,
    /**
     * The client has passed in an invalid pointer
     * i.e. one which lies outside the bounds of the current heap.
     */
    LUB_HEAP_INVALID_POINTER
} lub_heap_status_t;

typedef struct {void *ptr;} struct_void_ptr;
/**
 * This type is used to indicate the alignment required 
 * for a memory allocation.
 */
typedef enum
{
    /**
     * This is the "native" alignment required for the current
     * CPU architecture.
     */
    LUB_HEAP_ALIGN_NATIVE = sizeof(struct_void_ptr),
    /**
     * 4 byte alignment
     */
    LUB_HEAP_ALIGN_2_POWER_2 = 0x00000004,
    /**
     * 8 byte alignment
     */
    LUB_HEAP_ALIGN_2_POWER_3 = 0x00000008,
    /**
     * 16 byte alignment
     */
    LUB_HEAP_ALIGN_2_POWER_4 = 0x00000010,
    /**
     * 32 byte alignment
     */
    LUB_HEAP_ALIGN_2_POWER_5 = 0x00000020,
    /**
     * 64 byte alignment
     */
    LUB_HEAP_ALIGN_2_POWER_6 = 0x00000040,
    /**
     * 128 byte alignment
     */
    LUB_HEAP_ALIGN_2_POWER_7 = 0x00000080,
    /**
     * 256 byte alignment
     */
    LUB_HEAP_ALIGN_2_POWER_8 = 0x00000100,
    /**
     * 512 byte alignment
     */
    LUB_HEAP_ALIGN_2_POWER_9 = 0x00000200,
    /**
     * 1024 byte alignment (1KB)
     */
    LUB_HEAP_ALIGN_2_POWER_10 = 0x00000400,
    /**
     * 2048 byte alignment (2KB)
     */
    LUB_HEAP_ALIGN_2_POWER_11 = 0x00000800,
    /**
     * 4096 byte alignment (4KB)
     */
    LUB_HEAP_ALIGN_2_POWER_12 = 0x00001000,
    /**
     * 8192 byte alignment (8KB)
     */
    LUB_HEAP_ALIGN_2_POWER_13 = 0x00002000,
    /**
     * 16384 byte alignment (16KB)
     */
    LUB_HEAP_ALIGN_2_POWER_14 = 0x00004000,
    /**
     * 32768 byte alignment (32KB)
     */
    LUB_HEAP_ALIGN_2_POWER_15 = 0x00008000,
    /**
     * 65536 byte alignment (64KB)
     */
    LUB_HEAP_ALIGN_2_POWER_16 = 0x00010000,
    /**
     * 131072 byte alignment (128KB)
     */
    LUB_HEAP_ALIGN_2_POWER_17 = 0x00020000,
    /**
     * 262144 byte alignment (256KB)
     */
    LUB_HEAP_ALIGN_2_POWER_18 = 0x00040000,
    /**
     * 524288 byte alignment (512KB)
     */
    LUB_HEAP_ALIGN_2_POWER_19 = 0x00080000,
    /**
     * 1048576 byte alignment (1MB)
     */
    LUB_HEAP_ALIGN_2_POWER_20 = 0x00100000,
    /**
     * 2097152 byte alignment (2MB)
     */
    LUB_HEAP_ALIGN_2_POWER_21 = 0x00200000,
    /**
     * 4194304 byte alignment (4MB)
     */
    LUB_HEAP_ALIGN_2_POWER_22 = 0x00400000,
    /**
     * 8388608 byte alignment (8MB)
     */
    LUB_HEAP_ALIGN_2_POWER_23 = 0x00800000,
    /**
     *  16777216 byte alignment (16MB)
     */
    LUB_HEAP_ALIGN_2_POWER_24 = 0x01000000,
    /**
     *  33554432 byte alignment (32MB)
     */
    LUB_HEAP_ALIGN_2_POWER_25 = 0x02000000,
    /**
     * 67108864 byte alignment (64MB)
     */
    LUB_HEAP_ALIGN_2_POWER_26 = 0x04000000,
    /**
     * 134217728 byte alignment (128MB)
     */
    LUB_HEAP_ALIGN_2_POWER_27 = 0x08000000
} lub_heap_align_t;

/**
 * This type defines how leak details should be displayed
 */
typedef enum
{
    /**
     * Only show allocations which have no reference elsewhere
     * in the system
     */
    LUB_HEAP_SHOW_LEAKS,
    /**
     * Only show allocations which have no direct reference elsewhere
     * in the system, but do have their contents referenced.
     */
    LUB_HEAP_SHOW_PARTIALS,
    /**
     * Show all the current allocations in the system.
     */
    LUB_HEAP_SHOW_ALL
} lub_heap_show_e;

/**
 * This type defines a function prototype to be used to 
 * iterate around each of a number of things in the system.
 */
typedef void 
    lub_heap_foreach_fn(
        /**
         * The current entity
         */
        void *block,
        /**
         * A simple 1-based identifier for this entity
         */
        unsigned index,
        /**
         * The number of bytes available in this entity
         */
        size_t size,
        /**
         * Client specific argument
         */
        void *arg);

/**
 * This operation is a diagnostic which can be used to 
 * iterate around all the segments in the specified heap.
 * For example it may be desirable to output information about 
 * each of the segments present.
 *
 * \pre
 * - The heap needs to have been create with an initial memory segment.
 *
 * \return 
 * - none
 *
 * \post
 * -The specified function will have been called once for every segment
 *  in the specified heap
 */
void
    lub_heap_foreach_segment(
        /**
         * The heap instance on which to operate
         */
        lub_heap_t *instance,
        /**
         * The client provided function to call for each free block
         */
        lub_heap_foreach_fn *fn,
        /**
         * Some client specific data to pass through to the callback
         * function.
         */
        void *arg);
/**
 * This operation is a diagnostic which can be used to 
 * iterate around all the free blocks in the specified heap.
 * For example it may be desirable to output information about 
 * each of the free blocks present.
 *
 * \pre
 * - The heap needs to have been create with an initial memory segment.
 *
 * \return 
 * - none
 *
 * \post
 * -The specified function will have been called once for every free
 *  block in the specified heap
 */
void
    lub_heap_foreach_free_block(
        /**
         * The heap instance on which to operate
         */
        lub_heap_t *instance,
        /**
         * The client provided function to call for each free block
         */
        lub_heap_foreach_fn *fn,
        /**
         * Some client specific data to pass through to the callback
         * function.
         */
        void *arg);

/**
  * This operation creates a dynamic heap from the provided
  * memory segment.
  *
  * \pre
  * - none
  *
  * \return
  * - a reference to a heap object which can be used to allocate
  *   memory from the segments associated with this heap.
  *
  * \post
  * - memory allocations can be invoked on the returned intance.
  * - further memory segements can be augmented to the heap using
  *   the lub_heap_add_segment() operation.
  */
lub_heap_t *
    lub_heap_create(
        /**
         * The begining of the first memory segment to associate with 
         * this heap
         */
        void *start,
        /**
         * The number of bytes available for use in the first segment.
         */
        size_t size
    );
/**
  * This operation creates a dynamic heap from the provided
  * memory segment.
  *
  * \pre
 * - The heap needs to have been create with an initial memory segment.
  *
  * \return
  * - none
  *
  * \post
  * - The heap is no longer valid for use.
  * - The memory segment(s) previously given to the heap
  *   may now be reused.
  * - Any extra resources used for leak detection will have been released.
  */
void
    lub_heap_destroy(
        /**
         * The heap instance on which to operate
         */
        lub_heap_t *instance
    );
/**
 * This operation augments an existing heap with 
 * some more memory to manage.
 * NB. if the memory happens to be follow on from the initial memory segment
 * then the two will merge into a single larger segment. This means that a heap
 * which is expanded with a sbrk() like mechanism will contain a single
 * expandible segment.
 *
 * \pre
 * - The heap needs to have been create with an initial memory segment.
 *
 * \return 
 * - none
 *
 * \post
 * - The new segment of memory becomes available for use by this heap.
 */
void 
    lub_heap_add_segment(
        /**
         * The heap instance on which to operate
         */
        lub_heap_t *instance,
        /** 
         * The beginning of the memory segment to be managed
         */
        void *start,
        /**
         * The number of bytes available for use in this segment
         */
        size_t size
    );
/**
 * This operation allocates some "static" memory from a heap. This is 
 * memory which will remain allocted for the lifetime of the heap instance.
 * "static" memory allocation has zero overhead and causes zero fragmentation.
 *
 * NB. static allocation are only handed out from the first memory segment 
 *
 * \pre
 * - The heap needs to have been created with an initial memory segment.
 *
 * \return 
 * - a pointer to some "static" memory which will be fixed for the
 *   remaining lifetime of this heap.
 *
 * \post
 * - The client cannot ever free this memory although if the heap is
 *   managing memory which itself has been dynamically allocated, then 
 *   the memory will be recovered when the heap is released.
 */
void *
    lub_heap_static_alloc(
        /**
         * The heap instance on which to operate
         */
        lub_heap_t *instance,
        /**
         * The number of bytes to allocate
         */
        size_t size
    );
/**
 * This operation changes the size of the object referenced by a passed in 
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
lub_heap_status_t
    lub_heap_realloc(
        /**
         * The heap instance on which to operate
         */
        lub_heap_t *instance,
        /**
         * Reference to a pointer containing previously allocated memory 
         * or NULL.
         */
        char **ptr,
        /**
         * The number of bytes required for the object 
         */
        size_t size,
        /**
         * The alignement required for a new allocations.
         */
        lub_heap_align_t alignment
    );
/**
 * This operation controls the tainted memory facility.
 * This means that during certain heap operations memory can
 * be filled with some well defined bit patterns. This causes
 * a slight performance overhead but can be used to shake out 
 * bugs such and free-memory-reads and uninitialised-memory-reads
 * 
 * By default tainting is switched off.
 *
 * \pre
 * - none
 *
 * \return 
 * - last tainted status
 *
 * \post
 * - (enabled) when a memory segment is given to a heap (either at creation or later)
 *   the contents will be set to 0xBB
 * - (enabled) when some dynamically allocated memory is released back to a heap
 *   the contents will be set to 0xFF
 * - (enabled) when some dynamic or static memory is allocated the contents will
 *   be set to 0xAA as the "uninitialised" value.
 * - (disabled) no memory tainting will occur.
 */
bool_t
    lub_heap_taint(
        /**
         * BOOL_TRUE to enable tainting or BOOL_FALSE to disable
         */
        bool_t enable
    );
/**
 * This operation indicates the current status of the memory tainting
 * facility
 * 
 * \pre
 * none
 *
 * \return
 * - BOOL_TRUE if memory tainting is enabled.
 * - BOOL_FALSE if memory tainting is disabled.
 *
 * \post 
 * none
 */ 
bool_t
    lub_heap_is_tainting(void);
/**
 * This operation controls runtime heap corruption detection.
 * This means that during every heap operation a full check is 
 * done of the specified heap, before any allocations/free are 
 * performed.
 * This has a performance overhead but provides a valuable
 * aid in finding a memory corrupting client.
 * Corruption will be spotted the first time a memory operation
 * is performed AFTER it has occured.
 * 
 * By default checking is switched off.
 *
 * \pre
 * - none
 *
 * \return 
 * - last checking status
 *
 * \post
 * - (enabled) the heap will have been scanned for any corruption and if found the
 * return status of the invoking heap operation will be LUB_HEAP_CORRUPTED.
 * - (disabled) no entire heap memory checking will occur.
 */
bool_t
    lub_heap_check(
        /**
         * BOOL_TRUE to enable checking or BOOL_FALSE to disable
         */
        bool_t enable
    );

/**
 * This operation indicates the current status of the full memory checking
 * facility.
 * 
 * \pre
 * none
 *
 * \return
 * - BOOL_TRUE if full memory checking is enabled.
 * - BOOL_FALSE if full memory checking is disabled.
 *
 * \post 
 * none
 */ 
bool_t
    lub_heap_is_checking(void);

/**
 * This operation checks the integrety of the memory in the specified
 * heap.
 * Corruption will be spotted the first time a check is performed AFTER 
 * it has occured.
 * \pre
 * - the specified heap will have been created
 *
 * \return 
 * - BOOL_TRUE if the heap is OK
 * - BOOL_FALSE if the heap is corrupted.
 *
 * \post
 * - none
 */
extern bool_t 
    lub_heap_check_memory(lub_heap_t *instance);

/**
 * This function is invoked whenever a call to lub_heap_realloc() fails.
 * It is provided as a debugging aid; simple set a breakpoint to
 * stop execution of the program and any failures will be caught in context.
 */
void 
    lub_heap_stop_here(
        /**
         * The failure status of the the call to realloc
         */
        lub_heap_status_t status,
        /**
         * The old value of the pointer passed in
         */
        char *old_ptr,
        /**
         * The requested number of bytes
         */
        size_t new_size
    );

/**
 * This operation fills out a statistics structure with the details for the 
 * specified heap.
 *
 * \pre
 * - none
 *
 * \post
 * - the results filled out are a snapshot of the statistics as the time
 *   of the call.
 */
void
    lub_heap__get_stats(
        /**
         * The instance on which to operate
         */
        lub_heap_t *instance,
        /**
         * A client provided structure to fill out with the heap details
         */
        lub_heap_stats_t *stats
    );

/**
 * This operation dumps the salient details of the specified heap to stdout
 */
void
    lub_heap_show(
        /**
         * The instance on which to operate
         */
        lub_heap_t *instance,
        /**
         * Whether to be verbose or not
         */
        bool_t verbose
    );
/**
 * This method provides the size, in bytes, of the largest allocation
 * which can be performed.
 * \pre
 * - The heap needs to have been created with an initial memory segment.
 *
 * \return 
 * - size of largest possible allocation.
 *
 * \post
 * - none
 */
size_t    
lub_heap__get_max_free(
        /**
         * The instance on which to operate
         */
        lub_heap_t *instance
    );

extern size_t
    lub_heap__get_block_overhead(lub_heap_t *instance,
                                 const void *ptr);

extern size_t
    lub_heap__get_block_size(lub_heap_t *instance,
                             const void *ptr);
/**
 * This function scans memory to identify memory leaks
 * 
 * NB. if tainting is switched off then this function may miss some leaks as
 * references may remain in freed memory.
 *
 */
extern void
    lub_heap_leak_scan(void);
/**
 * This function dumps all the context details for the heap
 * to stdout.
 * 'how' is one of the following values:
 * 0 - show only leaks
 * 1 - show partial leaks (no references to the start of the block)
 * 2 - show all allocations (VERY VERBOSE)
 *
 * NB. if tainting is switched off then this function will not perform
 * any memory scanning and will simply show all the allocated blocks.
 *
 * \return 
 * - a boolean indicating whether any leaks were displayed or not.
 */
extern bool_t
    lub_heap_leak_report(
        /**
         * how to display the details
         */
        lub_heap_show_e how,
        /**
         * an optional substring to use to filter contexts.
         * Only contexts which contain the substring will be displayed
         */
        const char *substring
    );

void
    lub_heap__set_framecount(
        /**
         * The new framecount to use
         */
        unsigned framecount
    );

unsigned
    lub_heap__get_framecount(void);
    
extern bool_t 
    lub_heap_validate_pointer(lub_heap_t *instance,
                              char       *ptr);
/**
 * This 'magic' pointer is returned when a client requests zero bytes
 * The client can see that the allocation has succeeded, but cannot use 
 * the "memory" returned. This pointer may be passed transparently back
 * to lub_heap_realloc() without impact.
 */
#define LUB_HEAP_ZERO_ALLOC ((void*)-1)

/**
 * This operation is used to initialise the heap management 
 * subsystem
 * \pre
 * - none
 *
 * \post
 * - The POSIX specific subsystem will be initialised to load
 *   the debugging symbols for the current executable. This
 *   enables the backtraces used for leak detection to show
 *   the full detail in the stack traces.
 * - If the system is configured at build time without GPL
 *   support (disabled by default) then only the address of
 *   each stack frame will be shown.
 */
extern void
    lub_heap_init(
        /** 
         * The full pathname of the current executable
         * This is typically obtained from argv[0] in main()
         */
        const char *program_name
    );


#if defined(__CYGWIN__)

/**
 * CYGWIN requires a specialised initialisation to account for 
 * argv[0] not containing the trailing ".exe" of the executable.
 */
extern void 
    cygwin_lub_heap_init(const char *file_name);

#define lub_heap_init(arg0) cygwin_lub_heap_init(arg0)

#endif /* __CYGWIN__ */
/**
  * This operation adds a cache to the current heap, which speeds up
  * the allocation and releasing of smaller block sizes.
  *
  * \pre
  * - The heap must have been initialised
  * - This call must not have been made on this heap before
  *
  * \return
  * - LUB_HEAP_OK if the cache was successfully set up.
  * - LUB_HEAP_FAILED if the cache was not set up for whatever reason
  *
  * \post
  * - memory allocations for smaller block sizes may come from the
  *   cache.
  */
lub_heap_status_t
    lub_heap_cache_init(
        /**
         * The instance on which to operate
         */
        lub_heap_t *instance,
        /**
         * The maximum block size for the cache
         */
        lub_heap_align_t max_block_size,
        /**
         * The number of maximum sized blocks to make available.
         */
        size_t num_max_blocks
    );
/**
  * This operation signals the start of a section of code which 
  * should not have any of it's heap usage monitored by the leak
  * detection code.
  *
  * \pre
  * - The heap must have been initialised
  *
  * \return
  * - none
  *
  * \post
  * - If leak detection is enabled then no subsequent allocations will
  *   be monitored until the lub_heap_leak_restore_detection() is called.
  */
void
    lub_heap_leak_suppress_detection(
        /**
         * The instance on which to operate
         */
        lub_heap_t *instance
    );
/**
  * This operation signals the end of a section of code which 
  * should not have any of it's heap usage monitored by the leak
  * detection code.
  *
  * NB. you may nest the usage of lub_heap_leak_suppress_detection() and
  * lub_heap_leak_restore_detection() in which case only when the outermost
  * section has been terminated will monitoring commence again.
  *
  * \pre
  * - The heap must have been initialised
  * - lub_heap_start_unmonitored_section() must have been called.
  *
  * \return
  * - none
  *
  * \post
  * - If leak detection is enabled then no subsequent allocations will
  *   be monitored until the lub_heap_end_unmonitored_section() is called.
  */
void
    lub_heap_leak_restore_detection(
        /**
         * The instance on which to operate
         */
        lub_heap_t *instance
    );

/**
  * This operation returns the overhead, in bytes, which is required
  * to implement a heap instance. This provide clients the means of 
  * calculating how much memory they need to assign for a heap instance
  * to manage.
  *
  * \pre
  * - none
  *
  * \return
  * - size in bytes of the overhead required by a lub_heap instance.
  *
  * \post
  * - none
  */
size_t
    lub_heap_overhead_size(
        /**
         * The maximum block size for the cache
         */
        lub_heap_align_t max_block_size,
        /**
         * The number of maximum sized blocks to make available.
         */
        size_t num_max_blocks
    );


_END_C_DECL

#endif /* _lub_heap_h */
/** @} */
