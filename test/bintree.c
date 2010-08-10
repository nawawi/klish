#include <string.h>
#include <stdio.h>
#include "bintree.h"

#include "lub/blockpool.h"
#include "lub/test.h"
/**
 \example test/bintree.c
 */

/*************************************************************
 * TEST CODE
 ************************************************************* */
/* create a pool of nodes */
static test_node_t nodes[NUM_TEST_NODES];
static lub_blockpool_t testpool;

static test_node_t duplicate;

static int testseq;

/*--------------------------------------------------------------- */
/*
 #define DUMP
 */

static void
dump_node(lub_bintree_t      *this,
          lub_bintree_node_t *node)
{
        test_node_t *clientnode = (void *)(((char*)node) - this->node_offset);
        test_node_t *leftnode   = (void *)(((char*)node->left) - this->node_offset);
        test_node_t *rightnode  = (void *)(((char*)node->right) - this->node_offset);
        
        if(NULL != node)
        {
                printf(" %d<--%d-->%d",
                       leftnode ? leftnode->value : -1,
                       clientnode->value,
                       rightnode ? rightnode->value : -1);
                
                if(node->left)
                        dump_node(this,node->left);
                
                if(node->right)
                        dump_node(this,node->right);
        }
}

extern void dump_tree(lub_bintree_t *tree);
void
dump_tree(lub_bintree_t *tree)
{
        printf("\nTREE(0x%p):",(void*)tree);
        dump_node(tree,tree->root);
        printf("\n");
}
#ifndef DUMP
#define dump_tree(tree)
#endif /* not DUMP */

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

/* This operation is invokes to initialise a new instance of a test node
 *
 * seed - the seed to use for the random details within the instance
 */
static void
constructor(test_node_t *this,
            int          seed)
{
        this->value      = seed;
        this->address[0] = 0x08;
        this->address[1] = 0x00;
        this->address[2] = 0x4e;
        this->address[3] = 0x00;
        this->address[4] = ((this->value     ) & 0xFF);
        this->address[5] = ((this->value >> 8) & 0xFF);

        /* initialise the control blocks */
        lub_bintree_node_init(&this->bt_node1);
        lub_bintree_node_init(&this->bt_node2);
}
/* This operation creates and initialised a new instance of a test node
 *
 * seed - the seed to use for the random details within the instance
 */
static test_node_t *
new(int seed)
{
        test_node_t *node = lub_blockpool_alloc(&testpool);

        if(NULL != node)
        {
                /* call the constructor */
                constructor(node,seed);
        }
        return node;
}

/* forward declare these functions */
static lub_bintree_compare_fn test_value_compare;
static lub_bintree_getkey_fn  test_value_getkey;
static lub_bintree_compare_fn test_address_compare;
static lub_bintree_getkey_fn  test_address_getkey;

/*
 * This defines the attributes associated with a tree
 */
typedef struct
{
        lub_bintree_t           tree;
        lub_bintree_compare_fn *compare;
        lub_bintree_getkey_fn  *getkey;
        size_t                   offset;
        lub_bintree_key_t       nonexistant;
} mapping_t;


typedef struct
{
        unsigned char address[6];
} address_key_t;

typedef struct
{
        int value;
} value_key_t;


/* This is the main entry point for this executable
 */
int main(int argc, const char *argv[])
{
        unsigned int i,t;
 	int          status;
 	
        /*
         * TREE ATTRBUTES
         */
        address_key_t address_key = {{0xff,0xff,0xff,0xff,0xff,0xff}};
        value_key_t   value_key   = {-1};

        mapping_t     mapping[2];

	lub_test_parse_command_line(argc,argv);
	lub_test_begin("lub_bintree");

        /*
         * define the attributes for each tree
         */
        /* integer based search */
        mapping[0].compare     = test_value_compare;
        mapping[0].getkey      = test_value_getkey;
        mapping[0].offset      = offsetof(test_node_t,bt_node1);
        memcpy(&mapping[0].nonexistant,&value_key,sizeof(value_key));
        
        /* MAC address based search */
        mapping[1].compare     = test_address_compare;
        mapping[1].getkey      = test_address_getkey;
        mapping[1].offset      = offsetof(test_node_t,bt_node2);
        memcpy(&mapping[1].nonexistant,&address_key,sizeof(address_key));

        /*
         * create a duplicate node... (will get the same details as
         * the first one from the pool)
         */
        constructor(&duplicate,0);
        
        /*
         * create a blockpool to manage node allocation/deallocation
         */
        lub_blockpool_init(&testpool,
                            nodes,
                            sizeof(test_node_t),
                            NUM_TEST_NODES);
        /*
         * test each tree
         */ 
        for(t = 0; t < 2; t++)
        {
                lub_bintree_t *tree = &mapping[t].tree;
		
                /* create the tree */
		lub_test_seq_begin(++testseq,"tree(%d)",t);
                lub_bintree_init(tree,
                                  mapping[t].offset,
                                  mapping[t].compare,
                                  mapping[t].getkey);


 		/*------------------------------------------------------------ */
                lub_test_check(NULL == lub_bintree_findfirst(tree),
                              "Check findfirst returns NULL on empty tree");
		lub_test_check(NULL == lub_bintree_findlast(tree),
		              "Check findlast returns NULL on an empty tree");
 		/*------------------------------------------------------------ */
   		status = LUB_TEST_PASS;
                for(i=0; i < NUM_TEST_NODES;i++)
                {
                        test_node_t *node;
                        if(0 == t)
                        {
                                /* create a test node */
                                node = new(i);
				if(NULL == node)
				{
					status = LUB_TEST_FAIL;
					break;
                        	}
                        }
                        else
                        {
                                value_key_t key;
                                key.value = i;
                                /*
                                 * find the necessary node using the
                                 * first tree (fiddling with one tree
                                 * before inserting into another
                                 */
                                node = lub_bintree_find(&mapping[0].tree,
                                                         &key);
                        }
                        /* insert it into the tree */
                        if(0 != lub_bintree_insert(tree,node))
                        {
                        	status = LUB_TEST_FAIL;
                        	break;
                        }
                }
		lub_test_check(LUB_TEST_PASS == status,
		              "Inserting %d nodes",
		              NUM_TEST_NODES);
 		/*------------------------------------------------------------ */
		lub_test_check_int(-1,lub_bintree_insert(tree,&duplicate),
		                  "Inserting duplicate should fail");
 		/*------------------------------------------------------------ */
		lub_bintree_remove(tree,&duplicate);
		lub_test_seq_log(LUB_TEST_NORMAL,"Remove original node");
 		/*------------------------------------------------------------ */
		lub_test_check_int(0,lub_bintree_insert(tree,&duplicate),
		                  "Reinsert original node");
 		/*------------------------------------------------------------ */
		status = LUB_TEST_PASS;
                for(i=0; i < NUM_TEST_NODES;i++)
                {
                        test_node_t *node;
                        /*
                         * find each test node in the tree
                         */
                        if (t == 0) 
                        {
                                /*
                                 * search by integer
                                 */
                                value_key_t key;
                                key.value = random(i);
                                
                                node = lub_bintree_find(tree,&key);
				if(node->value != key.value)
				{
					status = LUB_TEST_FAIL;
					break;
				}
                        }
                        else
                        {
                                /*
                                 * search by address
                                 */
                                address_key_t key;
                                int rand = random(i);
                                key.address[0] = 0x08;
                                key.address[1] = 0x00;
                                key.address[2] = 0x4e;
                                key.address[3] = 0x00;
                                key.address[4] = ((rand     ) & 0xFF);
                                key.address[5] = ((rand >> 8) & 0xFF);
                                
                                node = lub_bintree_find(tree,&key);
                                if(memcmp(key.address, node->address,6) != 0)
                                {
                                	status = LUB_TEST_FAIL;
                                	break;
                                }
                        }
                }
		lub_test_check(LUB_TEST_PASS == status,
		              "Check lub_bintree_find() can find every node");
 		/*------------------------------------------------------------ */
                /*
                 * iterate through the tree forwards
                 */
                {
                        lub_bintree_iterator_t iter;
                        test_node_t *node = lub_bintree_findfirst(tree);
			int j = 0;

                	status = LUB_TEST_PASS;
                        for(lub_bintree_iterator_init(&iter,tree,node);
                            node;
                            node = lub_bintree_iterator_next(&iter),j++)
                        {
                        	if(t == 0)
                        	{
                        		/* check that the insertion works in the right order */
                        		if(node->value != j)
                        		{
                        			status = LUB_TEST_FAIL;
                        			break;
                        		}
                        	}
                        }
			lub_test_check(LUB_TEST_PASS == status,
				      "Iterate forwards checking order",t);
                }
 		/*------------------------------------------------------------ */
                /*
                 * iterate through the tree backwards
                 */
                {
                        lub_bintree_iterator_t iter;
                        test_node_t *node = lub_bintree_findlast(tree);
			int j = NUM_TEST_NODES - 1;            
                
                	status = LUB_TEST_PASS;
                        for(lub_bintree_iterator_init(&iter,tree,node);
                            node;
                            node = lub_bintree_iterator_previous(&iter),j--)
                        {
                        	if(t == 0)
                        	{
                        		/* check that the insertion works in the right order */
                        		if(node->value != j)
                        		{
                        			status = LUB_TEST_FAIL;
                        			break;
                        		}
                        	}
                        }
                }
		lub_test_check(LUB_TEST_PASS == status,
		              "Iterate backwards checking order",t);
 		/*------------------------------------------------------------ */
                lub_test_check(NULL == lub_bintree_find(tree,&mapping[t].nonexistant),
                              "Check search for non-existant node fails");
 		/*------------------------------------------------------------ */
		lub_test_seq_end();
        }

        for(t=0;t <2; t++)
        {
                lub_bintree_t *tree = &mapping[t].tree;
                test_node_t    *node;
                void           *(*find_op)(lub_bintree_t *) = lub_bintree_findfirst;
                
 		/*------------------------------------------------------------ */
                lub_test_seq_begin(++testseq,"tree(%d)",t);
                /*
                 * iterate through the tree removing the nodes
                 */
		i = 0;
                while( (node = find_op(tree)) )
                {
                        /* remove from the tree */
                        lub_bintree_remove(tree,node);
                        if (t == 1)
                        {
                                /* add back to the pool */
                                lub_blockpool_free(&testpool,node);
                        }
			i++;

			if(find_op == lub_bintree_findfirst)
			{
				find_op = lub_bintree_findlast;
			}
			else
			{
				find_op = lub_bintree_findfirst;
			}
                }
	        lub_test_check_int(i,NUM_TEST_NODES,"Check removed all nodes");
		lub_test_seq_end();
 		/*------------------------------------------------------------ */
        }
        
        /* tidy up */
        status = lub_test_get_status();
        lub_test_end();
        
        return status;
}

static int
test_value_compare(const void *clientnode,
                   const void *clientkey)
{
        const test_node_t *node = clientnode;
        const value_key_t *key  = clientkey;
        return node->value - key->value;
}

static void
test_value_getkey(const void         *clientnode,
                  lub_bintree_key_t *key)
{
        const test_node_t *node = clientnode;
        /* fill out the opaque key */
        memcpy(key,&node->value,sizeof(int));
}
static int
test_address_compare(const void *clientnode,
                     const void *clientkey)
{
        const test_node_t   *node = clientnode;
        return memcmp(node->address,clientkey,6);
}

static void
test_address_getkey(const void         *clientnode,
                    lub_bintree_key_t *key)
{
        const test_node_t *node = clientnode;
        /* fill out the opaque key */
        memcpy(key,node->address,6);
}

