/*********************** -*- Mode: C -*- ***********************
 * File            : blockpool_alloc.c
 *---------------------------------------------------------------
 * Description
 * ===========
 *---------------------------------------------------------------
 * This operation allocates a "block" of memory from a "blockpool"
 *
 * blockpool - the "blockpool" instance to invoke this operation upon
 *
 * PRE-CONDITIONS
 * The blockpool must have been initialised.
 * 
 * RETURNS
 * A pointer to a "block" of memory, or NULL if there is none left for
 * allocation.
 *
 * POST-CONDITIONS
 * The behaviour is undefined if the "blockpool" is uninitialised.
 *
 * If an initialisation operation has been registered then this will
 * have been invoked with the block pointer before returning from this
 * call. 
 *---------------------------------------------------------------
 * Author          : Graeme McKerrell
 * Created On      : Thu Jan 29 14:11:00 2004
 * Status          : TESTED
 *---------------------------------------------------------------
 * HISTORY
 * 7-Dec-2004		Graeme McKerrell	
 *    updated to use the "lub_" namespace
 * 5-May-2004		Graeme McKerrell	
 *    updates following review
 * 1-Apr-2004		Peter Kennedy	
 *    updated to fix last entry removal test - use tail
 * 6-Feb-2004		Graeme McKerrell	
 *    updated to use FIFO allocation of blocks
 *    removed initialisation function call.
 * 29-Jan-2004		Graeme McKerrell	
 *    Initial version
 *---------------------------------------------------------------
 * Copyright (C) 2004 3Com Corporation. All Rights Reserved.
 **************************************************************** */
#include "private.h"

/*--------------------------------------------------------- */
void *
lub_blockpool_alloc(lub_blockpool_t *this)
{
    lub_blockpool_block_t *newblock = this->m_head;
    if(newblock)
    {
        if(newblock == this->m_tail)
        {
            /* remove the last item from the pool */
            this->m_head = NULL;
            this->m_tail = NULL;
        }
        else
        {
            this->m_head = newblock->next;
        }
        /* updated the stats */
        ++this->m_alloc_blocks;
        ++this->m_alloc_total_blocks;
        if(this->m_alloc_blocks > this->m_alloc_hightide_blocks)
        {
            this->m_alloc_hightide_blocks = this->m_alloc_blocks;
        }
    }
    else
    {
        ++this->m_alloc_failures;
    }
    return newblock;
}
/*--------------------------------------------------------- */
