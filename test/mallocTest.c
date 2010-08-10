#include <stdlib.h>
#include <stdio.h>
#include <time.h>


int test_numbers[] =
{
    100,
    200,
    300,
    400,
    500,
    1000,
    2000,
    3000,
    4000,
    5000,
    10000,
    15000,
    20000,
    25000,
    30000,
    35000,
    40000,
    45000,
    50000,
    60000,
    70000,
    80000,
    90000,
    100000,
    200000,
    0
};

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
pseudo_random(int i)
{
        /* multiply the first prime after 2^14 and mask to 15 bits */
        return (16411*i) & (32767);
}
/*--------------------------------------------------------- */
static void
create_nodes(unsigned count,int verbose)
{
    test_node_t *node;
    int          i;
    
    if (verbose) printf("Create %d random sized blocks...",count);
    for(i = 0; i < count; i++)
    {
        /* pick a random size upto 4096 bytes */
        size_t size = 1 + 4096 * pseudo_random(i) / 32768;

        node = malloc(size);
        
        if(NULL != node)
        {
            /* put this into the linked list for later */
            node->next = first_node;
            first_node = node;
        }
        else
        {
            break;
        }
    }
    if (verbose) printf("done\n");
}
/*--------------------------------------------------------- */
static void
destroy_nodes(int count,
              int skip,
              int verbose)
{
    test_node_t **ptr = &first_node;
    int           i;
    
    if (verbose) printf("Fragment by destroying %d blocks...",count / skip);
    for(i = 0; 
        *ptr && (*ptr)->next && (i < count); 
        i++, ptr = &(*ptr)->next)
    {
        if(i % skip)
        {
            char *tmp = (char*)*ptr;
            /* remove from the linked list */
            *ptr = (*ptr)->next;

            /* and free the node */
            free(tmp);
        }
    }
    if (verbose) printf("done\n");
}
/*--------------------------------------------------------- */
void
mallocTest(int number)
{
    struct timespec start_time;
    struct timespec end_time;
    struct timespec delta_time;
    int            *num_allocs;
    int             single_shot[] =
    {
        0,0
    };
    if(0 == number)
    {
        num_allocs = test_numbers;
    }
    else
    {
        num_allocs = single_shot;
        single_shot[0] = number;
    }
    while(*num_allocs)
    {
        int count = *num_allocs++;
        
        clock_gettime(CLOCK_REALTIME,&start_time);
    
        create_nodes(count,single_shot[0]);
    
        if (single_shot[0]) printf("Now free every other node\n");
        destroy_nodes(count,2,single_shot[0]);
    
        create_nodes(count >> 1,single_shot[0]);
        if(single_shot[0])
        {
            printf("Now free all the nodes...");
        }
        while(first_node)
        {
            test_node_t *node = first_node;
            first_node = node->next;
            free(node);
        }
        if (single_shot[0]) printf("done\n");

        clock_gettime(CLOCK_REALTIME,&end_time);
        delta_time.tv_sec  = end_time.tv_sec  - start_time.tv_sec;
        delta_time.tv_nsec = end_time.tv_nsec - start_time.tv_nsec;

        printf("*** %6d in %10lu milliseconds\n",count,
                delta_time.tv_sec * 1000 + delta_time.tv_nsec/1000000);
    }
}
/*--------------------------------------------------------- */
int
main(int argc, char **argv)
{
    unsigned number = 0;

    if(argc > 1)
    {
        number = atoi(argv[1]);
    }
    mallocTest(number);
    
    return 0;
}
/*--------------------------------------------------------- */
