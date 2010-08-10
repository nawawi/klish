/*********************** -*- Mode: C -*- ***********************
 * File            : blockpool_init.c
 *---------------------------------------------------------------
 * Description
 * ===========
 * This operation initialises an instance of a blockpool.
 *
 * blockpool   - the "blockpool" instance to initialise.
 * memory      - the memory to be managed.
 * blocksize   - The size in bytes of each block.
 * blockcount  - The number of blocks to be managed. NB the client is
 *               responsible for ensuring that (blocksize x blockcount)
 *               bytes of memory are available for use.
 *
 * PRE-CONDITION
 * 'blocksize' must be an multiple of 'sizeof(void *)'.
 * (If the client declares a structure for the block this should be handled
 * automatically by the compiler)
 *
 * POST-CONDITION
 * If the size constraint is not met an assert will fire.
 *
 * Following initialisation the allocation of memory can be performed.
 *
 *---------------------------------------------------------------
 * Author          : Graeme McKerrell
 * Created On      : Thu Jan 29 13:57:54 2004
 * Status          : TESTED
 *---------------------------------------------------------------
 * HISTORY
 * 7-Dec-2004		Graeme McKerrell	
 *    updated to use the "lub_" namespace
 * 5-May-2004		Graeme McKerrell	
 *    updates following review
 * 6-Feb-2004		Graeme McKerrell	
 *    removed extra init_fn parameter from the initialisation call
 * 28-Jan-2004		Graeme McKerrell	
 *   Initial version
 *---------------------------------------------------------------
 * Copyright (C) 2004 3Com Corporation. All Rights Reserved.
 **************************************************************** */
#include <assert.h>
#include "private.h"

/*--------------------------------------------------------- */
void
lub_blockpool_init(lub_blockpool_t *this,
                   void            *memory,
                   size_t           blocksize,
                   unsigned         blockcount)
{
        unsigned i;
        char    *ptr = memory;
        
        /* check that this is a multiple of sizeof(void*) */
        assert((blocksize & (sizeof(void*)-1)) == 0);

        /* start off with nothing in the list */
        this->m_head = this->m_tail         = NULL;
    
        /* run through all the blocks placing them into the free list */
        for(i = 0;
            i < blockcount;
            ++i)
        {
                lub_blockpool_free(this,ptr);
                ptr += blocksize;
        }
        /* intialise the stats */
        this->m_block_size            = blocksize;
        this->m_num_blocks            = blockcount;
        this->m_alloc_blocks          = 0;
        this->m_alloc_total_blocks    = 0;
        this->m_alloc_hightide_blocks = 0;
        this->m_alloc_failures        = 0;
}
/*--------------------------------------------------------- */
