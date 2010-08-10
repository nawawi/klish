/*********************** -*- Mode: C -*- ***********************
 * File            : private.h
 *---------------------------------------------------------------
 * Description
 * ===========
 * This defines the private interface used internally by this component
 *---------------------------------------------------------------
 * Author          : Graeme McKerrell
 * Created On      : Wed Jan 28 08:45:01 2004
 * Status          : TESTED
 *---------------------------------------------------------------
 * HISTORY
 * 7-Dec-2004		Graeme McKerrell	
 *    updated to "lub_" namespace
 * 5-May-2004		Graeme McKerrell	
 *    updates following review
 * 28-Jan-2004		Graeme McKerrell	
 *   Initial version
 *---------------------------------------------------------------
 * Copyright (C) 2004 3Com Corporation. All Rights Reserved.
 **************************************************************** */
#include "lub/blockpool.h"

struct _lub_blockpool_block
{
    lub_blockpool_block_t *next;
};
/*************************************************************
 * PRIVATE OPERATIONS
 ************************************************************* */
