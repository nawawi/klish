/**
\ingroup lub
\defgroup lub_dblockpool dblockpool
 @{
 
\brief This interface provides a dynamic facility to manage the allocation and
deallocation of fixed sized blocks of memory.

It dynamically allocates chunks of memory each of which is used as a "blockpool".
This dynamic memory is taken from the standard heap via "malloc()".

This type of memory manager is very fast and doesn't suffer from
any memory fragmentation problems.

This allocator can reuse blocks in the order in which they are freed,
this is significant for some applications where you don't want to
re-use a particular freed block too soon. e.g. hardware timing
limits on re-use.

\author  Graeme McKerrell
\date    Created On      : Fri Feb 24 14:50:18 2005
\version TESTED
*/
/*---------------------------------------------------------------
 * HISTORY
 * 24-Feb-2006      Graeme McKerrell
 *    Initial version
 *---------------------------------------------------------------
 * Copyright (C) 2006 Newport Networks. All Rights Reserved.
 *--------------------------------------------------------------- */
#ifndef _lub_dblockpool_h
#define _lub_dblockpool_h
#include <stddef.h>

#include "c_decl.h"

_BEGIN_C_DECL

/****************************************************************
 * TYPE DEFINITIONS
 **************************************************************** */
/**
 * This type represents a "dblockpool" instance.
 */
typedef struct _lub_dblockpool lub_dblockpool_t;
/**
 * CLIENTS MUST NOT USE THESE FIELDS DIRECTLY
 */
struct _lub_dblockpool
{
    struct _lub_dblockpool_chunk *first_chunk;
    size_t                        block_size;
    unsigned                      chunk_size;
    unsigned                      max_chunks;
};

/****************************************************************
 * DBLOCKPOOL OPERATIONS
 **************************************************************** */
/**
 * This operation initialises an instance of a dblockpool.
 *
 * \pre 'blocksize' must be an multiple of 'sizeof(void *)'.
 * (If the client declares a structure for the block this should be handled
 * automatically by the compiler)
 *
 * \post If the size constraint is not met an assert will fire.
 * \post Following initialisation the allocation of memory can be performed.
 */
extern void 
    lub_dblockpool_init(
        /** 
         * the "dblockpool" instance to initialise.
         */
        lub_dblockpool_t *instance,
        /**
         * The size in bytes of each block.
         */
        size_t blocksize,
        /** 
         * The number of blocks to be managed in a chunk.
         */
        unsigned chunksize,
        /** 
         * The maximum number of chunks to be allocated.
         * (a value of zero means unlimited...)
         */
        unsigned max_chunks
    );
/**
 * This operation finalises an instance of a dblockpool.
 *
 * \pre 'instance' must have been initialised first.
 *
 * \post All the dynamic memory allocated will be released.
 */
extern void 
    lub_dblockpool_fini(
        /** 
         * the "dblockpool" instance to finalise.
         */
        lub_dblockpool_t *instance
    );
/**
 * This operation allocates a "block" of memory from a "dblockpool"
 *
 *
 * \pre
 * The dblockpool must have been initialised.
 * 
 * \return
 * A pointer to a "block" of memory, or NULL if there is none left for
 * allocation.
 *
 * \post
 * The behaviour is undefined if the "dblockpool" is uninitialised.
 */
extern void *
    lub_dblockpool_alloc(
        /** 
         * the "dblockpool" instance to operate on
         */
        lub_dblockpool_t *instance
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
    lub_dblockpool_free(
        /** 
         * the "blockpool" instance to invoke this operation upon
         */
        lub_dblockpool_t *instance,
        /** 
         * the "block" to release back to the pool
         */
        void *block
    );

_END_C_DECL

#endif /* _lub_dblockpool_h */
/** @} lub_dblockpool */
