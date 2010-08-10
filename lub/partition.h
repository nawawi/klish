/*
 * lub_partition.c
 */
 /**
\ingroup lub
\defgroup lub_heap heap
@{

\brief A thread safe dynamic memory alloction system for use in a multitasking
operating system.

This is a high level client of the lub_heap component and provides the following
additional features:

\section auto_resources Automatically obtains its resources
The client doesn't need to provide memory storage for this partition. It
will automatically obtain this from the system pool (malloc/free etc)

\section performance Multi-threading performance
If cache details are provided then a small high performance heap, 
just big enough to hold the cache, is created on a per thread 
basis. This is referenced using thread specific storage within the 
owning thread and if it is unable to satisfy the request then a
slower mutex locked global heap is created and used instead.

\section auto_extension Automatically extends itself
- The (slower) global heap will automatically extend itself as needed.

\author  Graeme McKerrell
\date    Created On      : Wed Jun 27 14:00:00 2007
\version UNTESTED
*/
#ifndef _lub_partition_h
#define _lub_partition_h
#include <stddef.h>

#include "lub/types.h"
#include "lub/c_decl.h"
#include "lub/heap.h"

_BEGIN_C_DECL

/**
 * This type is used to reference an instance of a heap.
 */
typedef struct _lub_partition lub_partition_t;


/**
 * This type defines a fundamental allocation function which 
 * can be used to extend the partition as needed.
 */
typedef void *lub_partition_sysalloc_fn(size_t required);
/**
 * This type is used to specify any local requirements
 */
typedef struct _lub_partition_spec lub_partition_spec_t;
struct _lub_partition_spec
{
    /**
     * Indicates whether or not to use a thread specific heap will be created
     * for each client of the partition.
     */
    bool_t use_local_heap;
    /**
     * The maximum block size for the local heap.
     */
    lub_heap_align_t max_local_block_size;
    /**
     * The number of maximum sized blocks to make available.
     *
     * If this is non zero then a local heap containing a cache with
     * (num_max_block * max_block_size) size buckets will be created
     *
     * If this is zero then a local heap of size max_block_size
     * will be created (without a cache)
     */
    size_t num_local_max_blocks;
    /**
     * When the partition grows each new segment will be at least this many bytes in size
     */
     size_t min_segment_size;
    /**
     * The limit in total bytes which can be allocated from the malloc hook
     * for the growth of this partition
     */
     size_t memory_limit;
    /**
     * If non-NULL then this pointer references the fundamental memory allocation
     * function which should be used to extend the partition.
     * If NULL then the standard 'malloc' function will be used. 
     */
     lub_partition_sysalloc_fn *sysalloc;
};

/**
  * This operation creates a partition
  *
  * \pre
  * - The system pool needs to be accessible
  *
  * \return
  * - a reference to a partition object which can be used to allocate
  *   memory
  *
  * \post
  * - memory allocations can be invoked on the returned intance.
  */
lub_partition_t *
    lub_partition_create(
        /**
         * This is used to specify the details to be used for 
         * the partition.
         */
        const lub_partition_spec_t *spec
    );
/**
  * This operation starts the process of killing a partition.
  *
  * \pre
 * - The partition needs to have been created.
  *
  * \return
  * - none
  *
  * \post
  * - The partition will no longer hand out memory.
  * - When the final outstanding piece of memory is handed back
  *   the partition will destroy itself.
  * - Upon final destruction any resources obtained from the
  *   system pool will be returned.
  */
void
    lub_partition_kill(
        /**
         * The heap instance on which to operate
         */
        lub_partition_t *instance
    );
/**
 * This operation changes the size of the object referenced by a passed in 
 * pointer to "size". The contents will be unchanged up to the minimum of the old 
 * and new sizes. If the new size is larger, the new space is uninitialised.
 *
 * \pre
 * - The partition needs to have been created.
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
    lub_partition_realloc(
        /**
         * The partition instance on which to operate
         */
        lub_partition_t *instance,
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
         * The alignment required for a new allocations.
         */
        lub_heap_align_t alignment
    );
/**
 * This operation checks the integrety of the memory in the specified
 * partition.
 * Corruption will be spotted the first time a check is performed AFTER 
 * it has occured.
 * \pre
 * - the specified partition will have been created
 *
 * \return 
 * - BOOL_TRUE if the partition is OK
 * - BOOL_FALSE if the partition is corrupted.
 *
 * \post
 * - none
 */
extern bool_t 
    lub_partition_check_memory(lub_partition_t *instance);
/**
 * This operation dumps the salient details of the specified partition to stdout
 */
void
    lub_partition_show(
        /**
         * The instance on which to operate
         */
        lub_partition_t *instance,
        /**
         * Whether to be verbose or not
         */
        bool_t verbose
    );
/**
 * This function is invoked whenever a call to lub_partition_realloc()
 * fails.
 * It is provided as a debugging aid; simple set a breakpoint to
 * stop execution of the program and any failures will be caught in context.
 */
void 
    lub_partition_stop_here(
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
 * This causes leak detection to be disabled for this partition 
 */
void
    lub_partition_disable_leak_detection(
        /**
         * The instance on which to operate
         */
        lub_partition_t *instance
    );
/**
 * This causes leak detection to be enabled for this partition
 */
void
    lub_partition_enable_leak_detection(
        /**
         * The instance on which to operate
         */
        lub_partition_t *instance
    );

_END_C_DECL

#endif /* _lub_partition_h */
/** @} */
