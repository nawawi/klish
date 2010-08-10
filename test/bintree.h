/*********************** -*- Mode: C -*- ***********************
 * Architecture    : 
 * SubSystem       : 
 * Component       : 
 * File            : bintree.h
 * Documents       : 
 * Components used : 
 *---------------------------------------------------------------
 * Description
 * ===========
 * This header defines a test node structure
 *---------------------------------------------------------------
 * Author          : Graeme McKerrell
 * Created On      : Wed Jan 28 13:14:36 2004
 * Status          : UNTESTED
 *---------------------------------------------------------------
 * HISTORY
 * 7-May-2004		Graeme McKerrell	
 *    updated to use "lub_test_" namespace
 *---------------------------------------------------------------
 * Copyright (c) 3Com Corporation. All Rights Reserved
 **************************************************************** */
#include "lub/bintree.h"
typedef struct
{
    lub_bintree_node_t bt_node1;
    lub_bintree_node_t bt_node2;
    int                value;
    unsigned char      address[6];
    short              idx; /* used by legacy implementation */
} test_node_t;

#define NUM_TEST_NODES 32768
