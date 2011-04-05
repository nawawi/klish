#ifndef _lub_list_h
#define _lub_list_h
#include <stddef.h>

/****************************************************************
 * TYPE DEFINITIONS
 **************************************************************** */

typedef struct lub_list_node_s lub_list_node_t;

/**
 * This type defines a callback function which will compare two nodes
 * with each other
 *
 * \param clientnode 	the client node to compare
 * \param clientkey 	the key to compare with a node
 *
 * \return
 *     <0 if clientnode  < clientkey;
 *      0 if clientnode == clientkey;
 *     >0 if clientnode  > clientkey
 */
typedef int lub_list_compare_fn(const void *first, const void *second);

/**
 * This type represents a list instance
 */
typedef struct lub_list_s lub_list_t;

/**
 * This is used to perform iterations of a list
 */
typedef struct lub_list_node_s lub_list_iterator_t;

/****************************************************************
 * LIST OPERATIONS
 **************************************************************** */
/**
 * This operation initialises an instance of a list.
 */
extern lub_list_t *lub_list_new(lub_list_compare_fn compareFn);

/**
 * This operation is called to initialise a "clientnode" ready for
 * insertion into a tree. This is only required once after the memory
 * for a node has been allocated.
 *
 * \pre none
 *
 * \post The node is ready to be inserted into a tree.
 */
extern lub_list_node_t *lub_list_node_new(void *data);

/*****************************************
 * NODE MANIPULATION OPERATIONS
 ***************************************** */
/**
 * This operation adds a client node to the specified tree.
 *
 * \pre The tree must be initialised
 * \pre The clientnode must be initialised
 * 
 * \return
 * 0 if the "clientnode" is added correctly to the tree.
 * If another "clientnode" already exists in the tree with the same key, then
 * -1 is returned, and the tree remains unchanged.
 *
 * \post If the bintree "node" is already part of a tree, then an
 * assert will fire.
 */

void lub_list_free(lub_list_t *list);
void lub_list_node_free(lub_list_node_t *node);
inline lub_list_node_t *lub_list__get_head(lub_list_t *list);
inline lub_list_node_t *lub_list__get_tail(lub_list_t *list);
lub_list_node_t *lub_list_node__get_prev(lub_list_node_t *node);
lub_list_node_t *lub_list_node__get_next(lub_list_node_t *node);
void *lub_list_node__get_data(lub_list_node_t *node);
lub_list_node_t *lub_list_iterator_init(lub_list_t *list);
lub_list_node_t *lub_list_iterator_next(lub_list_node_t *node);
lub_list_node_t *lub_list_iterator_prev(lub_list_node_t *node);
lub_list_node_t *lub_list_add(lub_list_t *list, void *data);
void lub_list_del(lub_list_t *list, lub_list_node_t *node);

#endif				/* _lub_list_h */

