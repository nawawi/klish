/**
\example test/heap.c
 */
#include <string.h>
#include <stdio.h>
#include "lub/heap.h"

#include "lub/test.h"

/*************************************************************
 * TEST CODE
 ************************************************************* */
int testseq = 0;

/* 
 * This is the main entry point for this executable
 */
#define SIZEOF_HEAP_T        (140)
#define SIZEOF_ALLOC_BLOCK_T (  8)
#define SIZEOF_NODE_T        ( 20)
#define SIZEOF_CONTEXT_T     (104)
 
#define RAW_SIZE (SIZEOF_HEAP_T + 132)

/* allow space to assign a leak detection context */
#define ACTUAL_SIZE (RAW_SIZE)

char segment1[ACTUAL_SIZE];
char segment2[ACTUAL_SIZE*4];

#define LARGE_SIZE 1024 * 400
char large_seg[LARGE_SIZE];

#define MIN_ALLOC 12

typedef struct test_node_s test_node_t;
struct test_node_s
{
    test_node_t *next;
};

static test_node_t *first_node;
/*--------------------------------------------------------- */
/*
 * Returns a pseudo random number based on the input.
 * This will given 2^15 concequtive numbers generate the entire
 * number space in a psedo random manner.
 */
static int
random(int i)
{
        /* multiply the first prime after 2^14 and mask to 15 bits */
        return (16411*i) & (32767);
}
/*--------------------------------------------------------- */
static void
create_nodes(lub_heap_t *heap,
             int         count)
{
    test_node_t *node;
    int          i;
    
    lub_test_seq_log(LUB_TEST_NORMAL,"Create % random sized blocks",count);
    first_node = NULL;
    for(i = 0; i < count; i++)
    {
        union
        {
            test_node_t **node_ptr;
            char        **char_ptr;
        } tmp;
        /* pick a random size upto 400 bytes */
        size_t size = 1 + 400 * random(i) / 32768;
        node = NULL;
        tmp.node_ptr = &node;
        lub_heap_realloc(heap,tmp.char_ptr,size,LUB_HEAP_ALIGN_NATIVE);
        
        if(NULL != node)
        {
            /* put this into the linked list for later */
            node->next = first_node;
            first_node = node;
        }
        else
        {
            lub_test_seq_log(LUB_TEST_NORMAL,"ONLY MANAGED TO CREATE %d BLOCKS",i);
            break;
        }
    }
    lub_test_seq_log(LUB_TEST_NORMAL,"Dump the details...");
    lub_heap_show(heap,BOOL_TRUE);
}
/*--------------------------------------------------------- */
static void
test_performance(void)
{
    lub_heap_t  *heap;
    
    lub_test_seq_begin(++testseq,"Check the performance...");
    heap = lub_heap_create(large_seg,sizeof(large_seg));
    lub_test_check(NULL != heap,"Check creation of 400KB heap");
    
    create_nodes(heap,1000);
    
    lub_test_seq_log(LUB_TEST_NORMAL,"Now free every other node...");
    {
        test_node_t **ptr;
        int         i   = 0;
        for(ptr = &first_node;
            *ptr;
            i++,ptr = &(*ptr)->next)
        {
            if(i % 2)
            {
                char *tmp = (char*)*ptr;
                /* remove from the linked list */
                *ptr = (*ptr)->next;
                
                /* and free the node */
                lub_heap_realloc(heap,&tmp,0,LUB_HEAP_ALIGN_NATIVE);
            }
        }
    }
    lub_test_seq_log(LUB_TEST_NORMAL,"Dump the details...");
    lub_heap_show(heap,BOOL_TRUE);

    create_nodes(heap,500);
    
    lub_heap_destroy(heap);
    lub_test_seq_end();
}
/*--------------------------------------------------------- */
static void
test_stats_check(lub_heap_t *this)
{
    lub_heap_stats_t stats;
    unsigned total_memory,used_memory;
    
    lub_heap__get_stats(this,&stats);
    total_memory = stats.segs_bytes 
                   - stats.static_bytes;
    used_memory  = stats.free_bytes 
                   + stats.free_overhead 
                   + stats.alloc_bytes 
                   + stats.alloc_overhead;
    
    lub_test_check_int(total_memory,used_memory,
                       "Check that the (segs_bytes - static_bytes) equals the free and allocated bytes + overheads");
    if(total_memory != used_memory)
    {
        lub_heap_show(this,BOOL_TRUE);
    }
}
/*--------------------------------------------------------- */
static void
test_taint_check(char *ptr,size_t size)
{
    bool_t tainted = BOOL_TRUE;
    if(ptr && (LUB_HEAP_ZERO_ALLOC != ptr))
    {
        while(size--)
        {
            if((unsigned char)*ptr != 0xaa)
            {
                tainted = BOOL_FALSE;
                break;
            }
        }
    }
    lub_test_check(tainted,"Check memory has been tainted");
}
/*--------------------------------------------------------- */
void
test_main(unsigned frame_count)
{
    lub_heap_t  *heap;
    char        *tmp;
    char        *ptr1 = NULL;
    char        *ptr2 = NULL;
    char        *ptr3 = NULL;
    lub_heap_status_t result;
    lub_heap_stats_t  stats;
    size_t       actual_size = RAW_SIZE;
    
    if(frame_count)
    {
        actual_size = ACTUAL_SIZE;
    }
    
    /*----------------------------------------------------- */
    lub_test_seq_begin(++testseq,"lub_heap_create(segment1,%d)",actual_size);
    
    heap = lub_heap_create(segment1,actual_size);
    lub_test_check(NULL != heap,"Check heap created correctly");
    lub_test_check(lub_heap_check_memory(heap),"Check integrity of heap");
    test_stats_check(heap);

    lub_test_seq_log(LUB_TEST_NORMAL,"Configured to collect %d frames for leak detection",frame_count);
    lub_heap__set_framecount(frame_count);

    lub_heap__get_stats(heap,&stats);
    lub_test_check_int(1,stats.free_blocks,"Check the number of free blocks");
    
    lub_test_seq_end();
    /*----------------------------------------------------- */
    
    lub_test_seq_begin(++testseq,"lub_heap_add_segment(segment2,%d)",actual_size*2);
    
    lub_heap_add_segment(heap,segment2,actual_size*2);
    lub_test_check(lub_heap_check_memory(heap),"Check integrity of heap");
    test_stats_check(heap);
    
    lub_heap__get_stats(heap,&stats);
    lub_test_check_int(1,stats.free_blocks,"Check the two adjacent segments get merged");
    
    lub_test_seq_end();
    /*----------------------------------------------------- */
    lub_test_seq_begin(++testseq,"lub_heap_static_alloc(heap,10)");
    
    tmp = lub_heap_static_alloc(heap,10);
    lub_test_check(lub_heap_check_memory(heap),"Check integrity of heap");
    lub_test_check(NULL != tmp,"Check static allocation works");
    test_taint_check(tmp,12);
    test_stats_check(heap);

    lub_heap__get_stats(heap,&stats);
    lub_test_check_int(1,stats.static_blocks,"Check the number of static blocks");
    lub_test_check_int(12,stats.static_bytes,"Check static_bytes has gone up");
    if (tmp) 
        memset(tmp,0x11,10);
    
    lub_test_seq_end();
    /*----------------------------------------------------- */
    lub_test_seq_begin(++testseq,"lub_heap_realloc(heap,&ptr1,10,LUB_HEAP_ALIGN_NATIVE)");
    
    result = lub_heap_realloc(heap,&ptr1,10,LUB_HEAP_ALIGN_NATIVE);
    lub_test_check(lub_heap_check_memory(heap),"Check integrity of heap");
    lub_test_check(LUB_HEAP_OK == result,"Check dynamic allocation says it works");
    lub_test_check(NULL != ptr1,"Check dynamic allocation actually works");
    test_taint_check(ptr1,10);
    test_stats_check(heap);
    
    lub_heap__get_stats(heap,&stats);
    lub_test_check_int(1,stats.alloc_blocks,"Check the number of dynamic blocks");
    lub_test_check_int(MIN_ALLOC,stats.alloc_bytes,"Check alloc bytes has gone up");
    if(ptr1)
        memset(ptr1,0x22,10);

    lub_test_seq_end();
    /*----------------------------------------------------- */
    lub_test_seq_begin(++testseq,"lub_heap_static_alloc(heap,10)");

    tmp = lub_heap_static_alloc(heap,10);
    lub_test_check(lub_heap_check_memory(heap),"Check integrity of heap");
    lub_test_check(NULL != tmp,"Check static allocation works");
    test_taint_check(tmp,10);
    test_stats_check(heap);
    
    lub_heap__get_stats(heap,&stats);
    lub_test_check_int(2,stats.static_blocks,"Check the number of static blocks");
    lub_test_check_int(2*MIN_ALLOC,stats.static_bytes,"Check static bytes has gone up");
    if(tmp)
        memset(tmp,0x33,10);

    lub_test_seq_end();
    /*----------------------------------------------------- */
    lub_test_seq_begin(++testseq,"lub_heap_static_alloc(heap,200)");
    
    tmp = lub_heap_static_alloc(heap,200);
    lub_test_check(lub_heap_check_memory(heap),"Check integrity of heap");
    lub_test_check(NULL != tmp,"Check static allocation from second segment works");
    test_taint_check(tmp,200);
    test_stats_check(heap);

    lub_heap__get_stats(heap,&stats);
    lub_test_check_int(3,stats.static_blocks,"Check the number of static blocks");
    lub_test_check_int(200+2*MIN_ALLOC,stats.static_bytes,"Check static_bytes has gone up");
    if(tmp)
        memset(tmp,0x99,200);
    
    lub_test_seq_end();
    /*----------------------------------------------------- */
    lub_test_seq_begin(++testseq,"lub_heap_realloc(heap,&ptr1,30,LUB_HEAP_ALIGN_NATIVE)");

    tmp = ptr1;
    result = lub_heap_realloc(heap,&ptr1,30,LUB_HEAP_ALIGN_NATIVE);
    lub_test_check(lub_heap_check_memory(heap),"Check integrity of heap");
    lub_test_check(LUB_HEAP_OK == result,"Check growing an allocation works");
    lub_test_check(ptr1 == tmp,"Check growing an allocation keeps the same pointer");
    test_taint_check(ptr1+10,20);
    test_stats_check(heap);
    
    lub_heap__get_stats(heap,&stats);
    lub_test_check_int(1,stats.alloc_blocks,"Check the number of dynamic blocks");
    lub_test_check_int(20 + MIN_ALLOC,stats.alloc_bytes,"Check alloc bytes has gone up");
    if(ptr1)
        memset(ptr1,0x44,30);

    lub_test_seq_end();
    /*----------------------------------------------------- */
    lub_test_seq_begin(++testseq,"lub_heap_realloc(heap,&ptr1,30,LUB_HEAP_ALIGN_NATIVE)");

    tmp = ptr1;
    result = lub_heap_realloc(heap,&ptr1,30,LUB_HEAP_ALIGN_NATIVE);
    lub_test_check(lub_heap_check_memory(heap),"Check integrity of heap");
    lub_test_check(LUB_HEAP_OK == result,"Check asking for the same size works");
    lub_test_check(ptr1 == tmp,"Check it keeps the same pointer");
    test_stats_check(heap);
    
    lub_heap__get_stats(heap,&stats);
    lub_test_check_int(1,stats.alloc_blocks,"Check the number of dynamic blocks");
    lub_test_check_int(20+MIN_ALLOC,stats.alloc_bytes,"Check alloc bytes");

    lub_test_seq_end();
    /*----------------------------------------------------- */
    lub_test_seq_begin(++testseq,"lub_heap_realloc(heap,&ptr1,4,LUB_HEAP_ALIGN_NATIVE)");

    tmp = ptr1;
    result = lub_heap_realloc(heap,&ptr1,4,LUB_HEAP_ALIGN_NATIVE);
    lub_test_check(lub_heap_check_memory(heap),"Check integrity of heap");
    lub_test_check(LUB_HEAP_OK == result,"Check shrinking an allocation works");
    lub_test_check(ptr1 == tmp,"Check shrinking an allocation retains pointer");
    test_taint_check(ptr1+4,MIN_ALLOC-4);
    test_stats_check(heap);
    
    lub_heap__get_stats(heap,&stats);
    lub_test_check_int(1,stats.alloc_blocks,"Check the number of dynamic blocks");
    if(ptr1)
        memset(ptr1,0x55,4);

    lub_test_seq_end();
    /*----------------------------------------------------- */
    lub_test_seq_begin(++testseq,"lub_heap_realloc(heap,&ptr1,100,LUB_HEAP_ALIGN_NATIVE)");

    tmp = ptr1;
    result = lub_heap_realloc(heap,&ptr1,104,LUB_HEAP_ALIGN_NATIVE);
    lub_test_check(lub_heap_check_memory(heap),"Check integrity of heap");
    lub_test_check(LUB_HEAP_OK == result,"Check growing and moving works");
    lub_test_check(ptr1 != tmp,"Check growing and moving changes the pointer");
    test_taint_check(ptr1+4,100);
    test_stats_check(heap);
    
    lub_heap__get_stats(heap,&stats);
    lub_test_check_int(1,stats.alloc_blocks,"Check the number of dynamic blocks");
    if(ptr1)
        memset(ptr1,0x55,100);

    lub_test_seq_end();
    /*----------------------------------------------------- */
    lub_test_seq_begin(++testseq,"lub_heap_realloc(heap,&ptr1,0,LUB_HEAP_ALIGN_NATIVE)");

    tmp = ptr1;
    result = lub_heap_realloc(heap,&ptr1,0,LUB_HEAP_ALIGN_NATIVE);
    lub_test_check(lub_heap_check_memory(heap),"Check integrity of heap");
    lub_test_check(LUB_HEAP_OK == result,"Check releasing an allocation says it works");
    lub_test_check(LUB_HEAP_ZERO_ALLOC == ptr1,"Check releasing an allocation actually works");
    test_stats_check(heap);
    
    lub_heap__get_stats(heap,&stats);
    lub_test_check_int(0,stats.alloc_blocks,"Check the number of dynamic blocks");
    lub_test_check(BOOL_TRUE == lub_heap_check_memory(heap),"Check integrity of heap");

    lub_test_seq_end();
    /*----------------------------------------------------- */
    lub_test_seq_begin(++testseq,"lub_heap_realloc(heap,&ptr1,20,LUB_HEAP_ALIGN_NATIVE)");

    /* now test with multiple dynamic blocks to exercise different recovery mechanisms */
    result = lub_heap_realloc(heap,&ptr1,20,LUB_HEAP_ALIGN_NATIVE);
    lub_test_check(lub_heap_check_memory(heap),"Check integrity of heap");
    lub_test_check(LUB_HEAP_OK == result,"Check dynamic allocation (1) says it works");
    lub_test_check(NULL != ptr1,"Check dynamic allocation (1) actually works");
    test_taint_check(ptr1,20);
    test_stats_check(heap);
    
    if(ptr1)
        memset(ptr1,0x66,20);

    lub_test_seq_end();
    /*----------------------------------------------------- */
    lub_test_seq_begin(++testseq,"lub_heap_realloc(heap,&ptr2,20,LUB_HEAP_ALIGN_NATIVE)");

    result = lub_heap_realloc(heap,&ptr2,30,LUB_HEAP_ALIGN_NATIVE);
    lub_test_check(lub_heap_check_memory(heap),"Check integrity of heap");
    lub_test_check(LUB_HEAP_OK == result,"Check dynamic allocation (2) says it works");
    lub_test_check(NULL != ptr2,"Check dynamic allocation (2) actually works");
    test_taint_check(ptr2,32);
    test_stats_check(heap);
    
    if(ptr2)
        memset(ptr2,0x77,20);

    lub_test_seq_end();
    /*----------------------------------------------------- */
    lub_test_seq_begin(++testseq,"lub_heap_realloc(heap,&ptr3,1,LUB_HEAP_ALIGN_NATIVE)");

    result = lub_heap_realloc(heap,&ptr3,1,LUB_HEAP_ALIGN_NATIVE);
    lub_test_check(lub_heap_check_memory(heap),"Check integrity of heap");
    lub_test_check(LUB_HEAP_OK == result,"Check dynamic allocation (3) says it works");
    lub_test_check(NULL != ptr3,"Check dynamic allocation (3) actually works");
    test_taint_check(ptr3,8);
    test_stats_check(heap);
    
    if(ptr3)
        memset(ptr3,0x88,1);

    lub_heap__get_stats(heap,&stats);
    lub_test_check_int(3,stats.alloc_blocks,"Check the number of dynamic blocks");
    lub_test_check_int(3,stats.static_blocks,"Check the number static blocks");
    lub_test_check_int(1,stats.free_blocks,"Check the number of free blocks");
    lub_test_check_int(60,stats.alloc_bytes,"Check alloc bytes has gone up");

    lub_test_seq_end();
    /*----------------------------------------------------- */
    lub_test_seq_begin(++testseq,"lub_heap_realloc(heap,&ptr2,8,LUB_HEAP_ALIGN_NATIVE)");

    /* shrink the second block to create a new free block */
    tmp = ptr2;
    result = lub_heap_realloc(heap,&ptr2,8,LUB_HEAP_ALIGN_NATIVE);
    lub_test_check(lub_heap_check_memory(heap),"Check integrity of heap");
    lub_test_check(LUB_HEAP_OK == result,"Check shrinking (2) says it works");
    lub_test_check(tmp == ptr2,"Check shrinking (2) maintains the pointer");
    test_stats_check(heap);
    
    lub_heap__get_stats(heap,&stats);
    lub_test_check_int(3,stats.alloc_blocks,"Check the number of dynamic blocks");
    lub_test_check_int(2,stats.free_blocks,"Check the number of free blocks");

    lub_test_seq_end();
    /*----------------------------------------------------- */
    lub_test_seq_begin(++testseq,"lub_heap_realloc(heap,&ptr2,0,LUB_HEAP_ALIGN_NATIVE)");

    /* release the second block to cause fragmentation */
    tmp = ptr2;
    result = lub_heap_realloc(heap,&ptr2,0,LUB_HEAP_ALIGN_NATIVE);
    lub_test_check(lub_heap_check_memory(heap),"Check integrity of heap");
    lub_test_check(LUB_HEAP_OK == result,"Check releasing an allocation (2) says it works");
    lub_test_check(LUB_HEAP_ZERO_ALLOC == ptr2,"Check releasing an allocation (2) actually works");
    test_stats_check(heap);
    
    lub_heap__get_stats(heap,&stats);
    lub_test_check_int(2,stats.alloc_blocks,"Check the number of dynamic blocks");
    lub_test_check_int(2,stats.free_blocks,"Check the number of free blocks");

    lub_test_seq_end();
    /*----------------------------------------------------- */
    lub_test_seq_begin(++testseq,"lub_heap_realloc(heap,&ptr2,0,LUB_HEAP_ALIGN_NATIVE)");

    /* deliberatly double free the memory */
    ptr2 = tmp;
    result = lub_heap_realloc(heap,&ptr2,0,LUB_HEAP_ALIGN_NATIVE);
    lub_test_check(lub_heap_check_memory(heap),"Check integrity of heap");
    lub_test_check(LUB_HEAP_DOUBLE_FREE == result,"Check double free is spotted");
    lub_test_check(tmp == ptr2,"Check pointer hasn't changed.");
    test_stats_check(heap);
    ptr2 = NULL;
    
    lub_heap__get_stats(heap,&stats);
    lub_test_check_int(2,stats.alloc_blocks,"Check the number of dynamic blocks");
    lub_test_check_int(2,stats.free_blocks,"Check the number of free blocks");

    lub_test_seq_end();
    /*----------------------------------------------------- */
    lub_test_seq_begin(++testseq,"lub_heap_realloc(heap,&ptr1,24,LUB_HEAP_ALIGN_NATIVE)");

    /* extend the lower block */
    tmp = ptr1;
    result = lub_heap_realloc(heap,&ptr1,24,LUB_HEAP_ALIGN_NATIVE);
    lub_test_check(lub_heap_check_memory(heap),"Check integrity of heap");
    lub_test_check(LUB_HEAP_OK == result,"Check growing allocation (1) says it works");
    lub_test_check(NULL != ptr1,"Check growing allocation (1) actually works");
    lub_test_check(tmp == ptr1,"Check growing allocation (1) didn't move block");
    test_taint_check(ptr1+20,4);
    test_stats_check(heap);
    
    lub_heap__get_stats(heap,&stats);
    lub_test_check_int(2,stats.alloc_blocks,"Check the number of dynamic blocks");
    lub_test_check_int(2,stats.free_blocks,"Check the number of free blocks");

    lub_test_seq_end();
    /*----------------------------------------------------- */
    lub_test_seq_begin(++testseq,"lub_heap_realloc(heap,&ptr1,48,LUB_HEAP_ALIGN_NATIVE)");

    /* extend the lower block to absorb the free block */
    tmp = ptr1;
    result = lub_heap_realloc(heap,&ptr1,48,LUB_HEAP_ALIGN_NATIVE);
    lub_test_check(lub_heap_check_memory(heap),"Check integrity of heap");
    lub_test_check(LUB_HEAP_OK == result,"Check growing allocation (1) says it works");
    lub_test_check(NULL != ptr1,"Check growing allocation (1) actually works");
    lub_test_check(tmp == ptr1,"Check growing allocation (1) didn't move block");
    test_taint_check(ptr1+20,16);
    test_stats_check(heap);
    
    lub_heap__get_stats(heap,&stats);
    lub_test_check_int(2,stats.alloc_blocks,"Check the number of dynamic blocks");
    lub_test_check_int(1,stats.free_blocks,"Check the number of free blocks");

    lub_test_seq_end();
    /*----------------------------------------------------- */
    lub_test_seq_begin(++testseq,"lub_heap_realloc(heap,&ptr1,0,LUB_HEAP_ALIGN_NATIVE)");

    /* release the lower block */
    tmp = ptr1;
    result = lub_heap_realloc(heap,&ptr1,0,LUB_HEAP_ALIGN_NATIVE);
    lub_test_check(lub_heap_check_memory(heap),"Check integrity of heap");
    lub_test_check(LUB_HEAP_OK == result,"Check releasing (1) says it works");
    lub_test_check(LUB_HEAP_ZERO_ALLOC == ptr1,"Check releasing memory (1) actually works");
    test_stats_check(heap);
    
    lub_heap__get_stats(heap,&stats);
    lub_test_check_int(1,stats.alloc_blocks,"Check the number of dynamic blocks");
    lub_test_check_int(2,stats.free_blocks,"Check the number of free blocks");

    lub_test_seq_end();
    /*----------------------------------------------------- */
    lub_test_seq_begin(++testseq,"lub_heap_realloc(heap,&ptr3,0,LUB_HEAP_ALIGN_NATIVE)");

    /* release the upper block */
    tmp = ptr3;
    result = lub_heap_realloc(heap,&ptr3,0,LUB_HEAP_ALIGN_NATIVE);
    lub_test_check(lub_heap_check_memory(heap),"Check integrity of heap");
    lub_test_check(LUB_HEAP_OK == result,"Check releasing (3) says it works");
    lub_test_check(LUB_HEAP_ZERO_ALLOC == ptr3,"Check releasing memory (3) actually works");
    test_stats_check(heap);
    
    lub_heap__get_stats(heap,&stats);
    lub_test_check_int(0,stats.alloc_blocks,"Check the number of dynamic blocks");
    lub_test_check_int(1,stats.free_blocks,"Check the number of free blocks");

    lub_test_seq_end();
    /*----------------------------------------------------- */
    lub_test_seq_begin(++testseq,"lub_heap_realloc(heap,&ptr1,12,LUB_HEAP_ALIGN_NATIVE)");

    /* now test extending blocks downwards */
    result = lub_heap_realloc(heap,&ptr1,12,LUB_HEAP_ALIGN_NATIVE);
    lub_test_check(lub_heap_check_memory(heap),"Check integrity of heap");
    lub_test_check(LUB_HEAP_OK == result,"Check allocation (1) says it works");
    lub_test_check(NULL != ptr1,"Check allocation memory (1) actually works");
    test_stats_check(heap);
    
    lub_heap__get_stats(heap,&stats);
    lub_test_check_int(1,stats.alloc_blocks,"Check the number of dynamic blocks");
    lub_test_check_int(1,stats.free_blocks,"Check the number of free blocks");

    lub_test_seq_end();
    /*----------------------------------------------------- */
    lub_test_seq_begin(++testseq,"lub_heap_realloc(heap,&ptr2,12,LUB_HEAP_ALIGN_NATIVE)");

    /* now test extending blocks downwards */
    result = lub_heap_realloc(heap,&ptr2,12,LUB_HEAP_ALIGN_NATIVE);
    lub_test_check(lub_heap_check_memory(heap),"Check integrity of heap");
    lub_test_check(LUB_HEAP_OK == result,"Check allocation (2) says it works");
    lub_test_check(NULL != ptr2,"Check allocation memory (2) actually works");
    test_stats_check(heap);
    
    lub_heap__get_stats(heap,&stats);
    lub_test_check_int(2,stats.alloc_blocks,"Check the number of dynamic blocks");
    lub_test_check_int(1,stats.free_blocks,"Check the number of free blocks");

    lub_test_seq_end();
    /*----------------------------------------------------- */
    lub_test_seq_begin(++testseq,"lub_heap_realloc(heap,&ptr3,12,LUB_HEAP_ALIGN_NATIVE)");

    /* now test extending blocks downwards */
    result = lub_heap_realloc(heap,&ptr3,12,LUB_HEAP_ALIGN_NATIVE);
    lub_test_check(lub_heap_check_memory(heap),"Check integrity of heap");
    lub_test_check(LUB_HEAP_OK == result,"Check allocation (1) says it works");
    lub_test_check(NULL != ptr3,"Check allocation memory (1) actually works");
    test_stats_check(heap);
    
    lub_heap__get_stats(heap,&stats);
    lub_test_check_int(3,stats.alloc_blocks,"Check the number of dynamic blocks");
    lub_test_check_int(1,stats.free_blocks,"Check the number of free blocks");

    lub_test_seq_end();
    /*----------------------------------------------------- */
    lub_test_seq_begin(++testseq,"lub_heap_realloc(heap,&ptr1,0,LUB_HEAP_ALIGN_NATIVE)");

    /* release the lower block */
    tmp = ptr1;
    result = lub_heap_realloc(heap,&ptr1,0,LUB_HEAP_ALIGN_NATIVE);
    lub_test_check(lub_heap_check_memory(heap),"Check integrity of heap");
    lub_test_check(LUB_HEAP_OK == result,"Check releasing (1) says it works");
    lub_test_check(LUB_HEAP_ZERO_ALLOC == ptr1,"Check releasing memory (1) actually works");
    test_stats_check(heap);
    
    lub_heap__get_stats(heap,&stats);
    lub_test_check_int(2,stats.alloc_blocks,"Check the number of dynamic blocks");
    lub_test_check_int(2,stats.free_blocks,"Check the number of free blocks");

    lub_test_seq_end();
    /*----------------------------------------------------- */
    lub_test_seq_begin(++testseq,"lub_heap_realloc(heap,&ptr2,16,LUB_HEAP_ALIGN_NATIVE)");

    /* now extend the middle block (downwards) */
    tmp = ptr2;
    result = lub_heap_realloc(heap,&ptr2,16,LUB_HEAP_ALIGN_NATIVE);
    lub_test_check(lub_heap_check_memory(heap),"Check integrity of heap");
    lub_test_check(LUB_HEAP_OK == result,"Check extending downwards (2) says it works");
    lub_test_check(tmp != ptr2,"Check extending downwards (2) changes pointer");
    test_stats_check(heap);
    
    lub_heap__get_stats(heap,&stats);
    lub_test_check_int(2,stats.alloc_blocks,"Check the number of dynamic blocks");
    lub_test_check_int(1,stats.free_blocks,"Check the number of free blocks");

    lub_test_seq_end();
    /*----------------------------------------------------- */
    lub_test_seq_begin(++testseq,"lub_heap_realloc(heap,&ptr2,20,LUB_HEAP_ALIGN_NATIVE)");

    /* now extend the middle block (downwards) */
    tmp = ptr2;
    result = lub_heap_realloc(heap,&ptr2,20,LUB_HEAP_ALIGN_NATIVE);
    lub_test_check(lub_heap_check_memory(heap),"Check integrity of heap");
    lub_test_check(LUB_HEAP_OK == result,"Check extending downwards (2) says it works");
    lub_test_check(tmp != ptr2,"Check extending downwards (2) changes pointer");
    test_stats_check(heap);
    
    lub_heap__get_stats(heap,&stats);
    lub_test_check_int(2,stats.alloc_blocks,"Check the number of dynamic blocks");
    lub_test_check_int(2,stats.free_blocks,"Check the number of free blocks");

    lub_test_seq_end();
    /*----------------------------------------------------- */
    lub_test_seq_begin(++testseq,"lub_heap_realloc(heap,&ptr2,0,LUB_HEAP_ALIGN_NATIVE)");

    /* now set the pointer to NULL */
    ptr2 = NULL;
    result = lub_heap_realloc(heap,&ptr2,0,LUB_HEAP_ALIGN_NATIVE);
    lub_test_check(lub_heap_check_memory(heap),"Check integrity of heap");
    lub_test_check(LUB_HEAP_OK == result,"Check that releasing NULL and allocating nothing says it works");
    lub_test_check(LUB_HEAP_ZERO_ALLOC == ptr2,"Check pointer is LUB_HEAP_ALLOC_ZERO");
    test_stats_check(heap);
    
    lub_heap__get_stats(heap,&stats);
    lub_test_check_int(2,stats.alloc_blocks,"Check the number of dynamic blocks");
    lub_test_check_int(1,stats.free_blocks,"Check the number of free blocks");

    lub_test_seq_end();
    /*----------------------------------------------------- */
    lub_test_seq_begin(++testseq,"lub_heap_realloc(heap,&ptr2,-1,LUB_HEAP_ALIGN_NATIVE)");

    result = lub_heap_realloc(heap,&ptr2,-1,LUB_HEAP_ALIGN_NATIVE);
    lub_test_check(lub_heap_check_memory(heap),"Check integrity of heap");
    lub_test_check(LUB_HEAP_FAILED == result,"Check that allocating large size near 2^32 fails");
    lub_test_check(NULL == ptr2,"Check pointer is unchanged");
    test_stats_check(heap);
    
    lub_heap__get_stats(heap,&stats);
    lub_test_check_int(2,stats.alloc_blocks,"Check the number of dynamic blocks");
    lub_test_check_int(1,stats.free_blocks,"Check the number of free blocks");

    lub_test_seq_end();
    /*----------------------------------------------------- */
    lub_test_seq_begin(++testseq,"lub_heap_show()");

    lub_heap_show(heap,BOOL_TRUE);
    
    lub_test_seq_end();
    /*----------------------------------------------------- */
    lub_test_seq_begin(++testseq,"lub_heap_show()");

    lub_heap_show(heap,BOOL_TRUE);
    
    lub_test_seq_end();
    /*----------------------------------------------------- */
    lub_test_seq_begin(++testseq,"lub_heap_show_leaks()");
    lub_heap_leak_report(LUB_HEAP_SHOW_ALL,"");
    lub_test_seq_end();
    /*----------------------------------------------------- */
    test_performance();
    /*----------------------------------------------------- */
    lub_test_seq_begin(++testseq,"lub_heap_destroy()");
    lub_heap_destroy(heap);
    lub_test_seq_end();
    /*----------------------------------------------------- */
}
/*--------------------------------------------------------- */
int 
main(int argc, const char *argv[])
{
    int status;

    lub_heap_init(argv[0]);
    
    lub_test_parse_command_line(argc,argv);
    lub_test_begin("lub_heap");

    lub_heap_taint(BOOL_TRUE);
    lub_heap_check(BOOL_TRUE);

    /* first of all test with leak detection switched off */
    test_main(0);

#if 1
    /* now test with leak detection switched on */
    test_main(1); /* a single context to use */
#endif
    /* tidy up */
    status = lub_test_get_status();
    lub_test_end();
    
    return status;
}
/*--------------------------------------------------------- */
