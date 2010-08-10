/**
\ingroup lub
\defgroup lub_blockpool blockpool
 @{
 
\brief This interface provides a facility to manage the allocation and
deallocation of fixed sized blocks of memory.

This type of memory manager is very fast and doesn't suffer from
any memory fragmentation problems.

This allocator uses block in the order in which they are freed,
this is significant for some applications where you don't want to
re-use a particular freed block too soon. e.g. hardware timing
limits on re-use.

The client is responsible for providing the memory to be managed.
\author  Graeme McKerrell
\date    Created On      : Fri Jan 23 12:50:18 2004
\version TESTED
*/
/*---------------------------------------------------------------
 * HISTORY
 * 7-Dec-2004		Graeme McKerrell	
 *    Updated to use the "lub" prefix
 * 6-Feb-2004		Graeme McKerrell	
 *    removed init_fn type definition and parameter, the client had
 *    more flexiblity in defining their own initialisation operation with
 *    arguments rather than use a "one-size-fits-all" approach.
 *    Modified blockpool structure to support FIFO block allocation.
 * 23-Jan-2004		Graeme McKerrell	
 *    Initial version
 *---------------------------------------------------------------
 * Copyright (C) 2004 3Com Corporation. All Rights Reserved.
 *--------------------------------------------------------------- */
#ifndef _lub_blockpool_h
#define _lub_blockpool_h
#include <stddef.h>

/****************************************************************
 * TYPE DEFINITIONS
 **************************************************************** */
typedef struct _lub_blockpool_block lub_blockpool_block_t;
/**
 * This type represents a "blockpool" instance.
 */
typedef struct _lub_blockpool lub_blockpool_t;
struct _lub_blockpool
{
    /* CLIENTS MUSTN'T TOUCH THESE DETAILS */
    lub_blockpool_block_t *m_head;
    lub_blockpool_block_t *m_tail;
    size_t                 m_block_size;
    size_t                 m_num_blocks;
    unsigned               m_alloc_blocks;
    unsigned               m_alloc_total_blocks;
    unsigned               m_alloc_hightide_blocks;
    unsigned               m_alloc_failures;
};
/**
 * This type defines the statistics available for each blockpool.
 */
typedef struct _lub_blockpool_stats lub_blockpool_stats_t;
struct _lub_blockpool_stats
{
    /*----------------------------------------------------- */
    /**
     * NUmber of bytes in each block.
     */
    size_t block_size;
    /**
     * Total number of blocks in this pool.
     */
    size_t num_blocks;
    /*----------------------------------------------------- */
    /**
     * Number of dynamically allocated blocks currently
     * held by clients of a blockpool.
     */
    size_t alloc_blocks;
    /**
     * Number of dynamically allocated bytes currently
     * held by clients of a blockpool.
     */
    size_t alloc_bytes;
    /**
     * Number of free blocks.
     */
    size_t free_blocks;
    /**
     * Number of bytes available in a blockpool.
     */
    size_t free_bytes;
    /*----------------------------------------------------- */
    /**
     * Cumulative number of dynamically allocated blocks
     * given to clients of a blockpool.
     */
    size_t alloc_total_blocks;
    /**
     * Cumulative number of dynamically allocated bytes
     * given to clients of a blockpool.
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
     * given to clients of a blockpool when the memory usage was at it's
     * highest
     */
    size_t alloc_hightide_bytes;
    /**
     * Number of free blocks when the memory usage was at it's
     * highest
     */
    size_t free_hightide_blocks;
    /**
     * Number of free bytes when the memory usage was at it's highest.
     */
    size_t free_hightide_bytes;
    /*----------------------------------------------------- */
    /**
     * Number of time an allocation has failed from this block
     */
    size_t alloc_failures;
    /*----------------------------------------------------- */
};
/****************************************************************
 * BLOCKPOOL OPERATIONS
 **************************************************************** */
/**
 * This operation initialises an instance of a blockpool.
 *
 *
 * \pre 'blocksize' must be an multiple of 'sizeof(void *)'.
 * (If the client declares a structure for the block this should be handled
 * automatically by the compiler)
 *
 * \post If the size constraint is not met an assert will fire.
 * \post Following initialisation the allocation of memory can be performed.
 */
extern void
    lub_blockpool_init(
        /** 
         * the "blockpool" instance to initialise.
         */
        lub_blockpool_t *blockpool,
        /** 
         * the memory to be managed.
         */
        void *memory,
        /**
         * The size in bytes of each block.
         */
        size_t blocksize,
        /** The number of blocks to be managed. NB the client is
         * responsible for ensuring that (blocksize x blockcount)
         * bytes of memory are available for use.
         */
        unsigned blockcount
    );

/**
 * This operation allocates a "block" of memory from a "blockpool"
 *
 *
 * \pre
 * The blockpool must have been initialised.
 * 
 * \return
 * A pointer to a "block" of memory, or NULL if there is none left for
 * allocation.
 *
 * \post
 * The behaviour is undefined if the "blockpool" is uninitialised.
 */
extern void *
    lub_blockpool_alloc(
        /** 
         * the "blockpool" instance to invoke this operation upon
         */
        lub_blockpool_t *blockpool
    );

/**
 * This operation de-allocates a "block" of memory back into a "blockpool"
 *
 * \pre
 * The specified block must have been previously allocated using
 * lub_blockpool_alloc()
 * 
 * \post
 * The de-allocated block become available for subsequent
 * lub_blockpool_alloc() requests.
 */
extern void
    lub_blockpool_free(
        /** 
         * the "blockpool" instance to invoke this operation upon
         */
        lub_blockpool_t *blockpool,
        /** 
         * the "block" to release back to the pool
         */
        void *block
    );
/**
 * This operation fills out a statistics structure with the details for the 
 * specified blockpool.
 *
 * \pre
 * - none
 *
 * \post
 * - the results filled out are a snapshot of the statistics as the time
 *   of the call.
 */
void
    lub_blockpool__get_stats(
        /**
         * The instance on which to operate
         */
        lub_blockpool_t *instance,
        /**
         * A client provided structure to fill out with the blockpool details
         */
        lub_blockpool_stats_t *stats
    );

#endif /* _lub_blockpool_h */
/** @} blockpool */

