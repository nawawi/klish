/** @file list.c
 * @brief Implementation of a bidirectional list.
 *
 * Bidirectional List stores special structures (nodes) as its elements.
 * Nodes are linked to each other. Node stores abstract user data (i.e. void *).
 *
 * List can be sorted or unsorted. To sort list user provides special callback
 * function to compare two nodes. The list will be sorted
 * due to this function return value that indicates "less than",
 * "equal", "greater than". Additionally user may provide another callback
 * function to free user defined data on list freeing.
 */

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "private.h"
#include "faux/list.h"


/** @brief Allocates and initializes new list node instance.
 *
 * @param [in] data User defined data to store within node.
 * @return Newly created list node instance or NULL on error.
 */
static faux_list_node_t *faux_list_new_node(void *data) {

	faux_list_node_t *node = NULL;

	node = faux_zmalloc(sizeof(*node));
	assert(node);
	if (!node)
		return NULL;

	// Initialize
	node->prev = NULL;
	node->next = NULL;
	node->data = data;

	return node;
}


/** @brief Free list node instance.
 *
 * @param [in] node List node instance.
 */
static void faux_list_free_node(faux_list_node_t *node) {

	assert(node);
	faux_free(node);
}


/** @brief Gets previous list node.
 *
 * @param [in] this List node instance.
 * @return List node previous in list.
 */
faux_list_node_t *faux_list_prev_node(const faux_list_node_t *node) {

	assert(node);
	if (!node)
		return NULL;

	return node->prev;
}


/** @brief Gets next list node.
 *
 * @param [in] this List node instance.
 * @return List node next in list.
 */
faux_list_node_t *faux_list_next_node(const faux_list_node_t *node) {

	assert(node);
	if (!node)
		return NULL;

	return node->next;
}


/** @brief Gets user data from list node.
 *
 * @param [in] this List node instance.
 * @return User data stored within specified list node.
 */
void *faux_list_data(const faux_list_node_t *node) {

	assert(node);
	if (!node)
		return NULL;

	return node->data;
}


/** @brief Iterate through each list node.
 *
 * On each call to this function the iterator will change its value.
 * Before function using the iterator must be initialised by list head node.
 *
 * @param [in,out] iter List node ptr used as an iterator.
 * @return List node or NULL if list elements are over.
 */
faux_list_node_t *faux_list_each_node(faux_list_node_t **iter) {

	faux_list_node_t *current_node = *iter;

	if (!current_node)
		return NULL;
	*iter = faux_list_next_node(current_node);

	return current_node;
}


/** @brief Iterate through each list node and returns user data.
 *
 * On each call to this function the iterator will change its value.
 * Before function using the iterator must be initialised by list head node.
 *
 * @param [in,out] iter List node ptr used as an iterator.
 * @return User data or NULL if list elements are over.
 */
void *faux_list_each(faux_list_node_t **iter) {

	faux_list_node_t *current_node = NULL;

	if (!*iter)
		return NULL;
	current_node = faux_list_each_node(iter);

	return faux_list_data(current_node);
}


/** @brief Allocate and initialize bidirectional list.
 *
 * Prototypes for callback functions:
 * @code
 * int faux_list_compare_fn(const void *first, const void *second);
 * void faux_list_free_fn(void *data);
 * @endcode
 *
 * @param [in] compareFn Callback function to compare two user data instances
 * to sort list.
 * @param [in] freeFn Callback function to free user data.
 * @return Newly created bidirectional list or NULL on error.
 */
faux_list_t *faux_list_new(faux_list_compare_fn compareFn,
	faux_list_free_fn freeFn) {

	faux_list_t *list = NULL;

	list = faux_zmalloc(sizeof(*list));
	assert(list);
	if (!list)
		return NULL;

	// Initialize
	list->head = NULL;
	list->tail = NULL;
	list->compareFn = compareFn;
	list->freeFn = freeFn;
	list->len = 0;

	return list;
}


/** @brief Free bidirectional list
 *
 * Free all nodes and user data from list and finally
 * free the list itself. It uses special callback
 * function specified by user (while faux_list_new()) to free the abstract
 * user data.
 *
 * @param [in] list List to free.
 */
void faux_list_free(faux_list_t *list) {

	faux_list_node_t *iter = NULL;

	assert(list);
	if (!list)
		return;

	while ((iter = faux_list_head(list))) {
		faux_list_del(list, iter);
	}
	faux_free(list);
}


/** @brief Gets head of list.
 *
 * @param [in] list List.
 * @return List node first in list.
 */
faux_list_node_t *faux_list_head(const faux_list_t *list) {

	assert(list);
	if (!list)
		return NULL;

	return list->head;
}


/** @brief Gets tail of list.
 *
 * @param [in] list List.
 * @return List node last in list.
 */
faux_list_node_t *faux_list_tail(const faux_list_t *list) {

	assert(list);
	if (!list)
		return NULL;

	return list->tail;
}


/** @brief Gets current length of list.
 *
 * @param [in] list List.
 * @return Current length of list.
 */
size_t faux_list_len(const faux_list_t *list) {

	assert(list);
	if (!list)
		return 0;

	return list->len;
}


/** @brief Generic static function for adding new list nodes.
 *
 * @param [in] list List to add node to.
 * @param [in] data User data for new list node.
 * @param [in] uniq - true/false Don't add entry with identical order
 * key (when the compareFn() returns 0)
 * @param [in] find - true/false Function returns list node if there is
 * identical entry. Or NULL if find is false.
 * @return Newly added list node.
 */
static faux_list_node_t *faux_list_add_generic(faux_list_t *list, void *data,
	bool_t uniq, bool_t find) {

	faux_list_node_t *node = NULL;
	faux_list_node_t *iter = NULL;

	assert(list);
	assert(data);
	if (!list || !data)
		return NULL;

	node = faux_list_new_node(data);
	if (!node)
		return NULL;

	// Empty list.
	if (!list->head) {
		list->head = node;
		list->tail = node;
		list->len++;
		return node;
	}

	// Not sorted list. Add to the tail.
	if (!list->compareFn) {
		node->prev = list->tail;
		node->next = NULL;
		list->tail->next = node;
		list->tail = node;
		list->len++;
		return node;
	}

	// Sorted list.
	iter = list->tail;
	while (iter) {
		int res = list->compareFn(node->data, iter->data);

		if (uniq && (res == 0)) {
			faux_list_free_node(node);
			return (find ? iter : NULL);
		}
		if (res >= 0) {
			node->next = iter->next;
			node->prev = iter;
			iter->next = node;
			if (node->next)
				node->next->prev = node;
			break;
		}
		iter = iter->prev;
	}
	// Insert node into the list head
	if (!iter) {
		node->next = list->head;
		node->prev = NULL;
		list->head->prev = node;
		list->head = node;
	}
	if (!node->next)
		list->tail = node;
	list->len++;

	return node;
}


/** @brief Adds user data (not unique) to the list.
 *
 * The user data is not unique. It means that two equal user data instances
 * can be added to the list.
 *
 * @param [in] list List to add entry to.
 * @param [in] data User data.
 * @return Newly created list node or NULL on error.
 */
faux_list_node_t *faux_list_add(faux_list_t *list, void *data) {

	return faux_list_add_generic(list, data, BOOL_FALSE, BOOL_FALSE);
}


/** @brief Adds user data (unique) to the list.
 *
 * The user data must be unique. It means that two equal user data instances
 * can not be added to the list. Function will return NULL if equal user
 * data is already in the list.
 *
 * @param [in] list List to add entry to.
 * @param [in] data User data.
 * @return Newly created list node or NULL on error.
 */
faux_list_node_t *faux_list_add_uniq(faux_list_t *list, void *data) {

	return faux_list_add_generic(list, data, BOOL_TRUE, BOOL_FALSE);
}


/** @brief Adds user data (unique) to the list or return equal existent node.
 *
 * The user data must be unique in this case. Function compares list nodes
 * with the new one. If equal node is already in the list then function
 * returns this node. Else new unique node will be added to the list.
 *
 * @param [in] list List to add entry to.
 * @param [in] data User data.
 * @return Newly created list node, existent equal node or NULL on error.
 */
faux_list_node_t *faux_list_add_find(faux_list_t *list, void *data) {

	return faux_list_add_generic(list, data, BOOL_TRUE, BOOL_TRUE);
}


/** Takes away list node from the list.
 *
 * Function removes list node from the list and returns user data
 * stored in this node.
 *
 * @param [in] list List to take away node from.
 * @param [in] node List node to take away.
 * @return User data from removed node or NULL on error.
 */
void *faux_list_takeaway(faux_list_t *list, faux_list_node_t *node) {

	void *data = NULL;

	assert(list);
	assert(node);
	if (!list || !node)
		return NULL;

	if (node->prev)
		node->prev->next = node->next;
	else
		list->head = node->next;
	if (node->next)
		node->next->prev = node->prev;
	else
		list->tail = node->prev;
	list->len--;

	data = faux_list_data(node);
	faux_list_free_node(node);

	return data;
}


/** @brief Deletes list node from the list.
 *
 * Functions removes node from the list and free user data memory if
 * freeFn callback was defined while list creation. If freeFn callback
 * is not defined then function is the same as faux_list_takeaway().
 *
 * @param [in] list List to delete node from.
 * @param [in] node List node to delete.
 * @return 0 on success, < 0 on error.
 */
int faux_list_del(faux_list_t *list, faux_list_node_t *node) {

	void *data = NULL;

	assert(list);
	assert(node);
	if (!list || !node)
		return -1;

	data = faux_list_takeaway(list, node);
	assert(data);
	if (!data) // Illegal case
		return -1;
	if (list->freeFn)
		list->freeFn(data);

	return 0;
}


/** @brief Search list for matching (match function).
 *
 * Function iterates through the list and executes special matching user defined
 * callback function matchFn for every list node. User can provide "userkey" -
 * the data that matchFn can use how it wants. The matchFn is arbitrary
 * argument. The userkey argument can be NULL. The function will immediately
 * return matched list node. To continue searching the saveptr argument contains
 * current iterator. So user can call to faux_list_match_node() for several
 * times and gets all matched nodes from list.
 *
 * Prototype for matchFn callback function:
 * @code
 * int faux_list_match_fn(const void *key, const void *data);
 * @endcode
 *
 * @param [in] list List.
 * @param [in] matchFn User defined matching callback function.
 * @param [in] userkey User defined data to use in matchFn function.
 * @param [in,out] saveptr Ptr to save iterator.
 * @return Matched list node.
 */
faux_list_node_t *faux_list_match_node(const faux_list_t *list,
	faux_list_match_fn matchFn, const void *userkey,
	faux_list_node_t **saveptr) {

	faux_list_node_t *iter = NULL;

	assert(list);
	assert(matchFn);
	if (!list || !matchFn || !list->head)
		return NULL;
	if (saveptr)
		iter = *saveptr;
	if (!iter)
		iter = list->head;
	while (iter) {
		int res;
		faux_list_node_t *node = iter;

		iter = faux_list_next_node(iter);
		if (saveptr)
			*saveptr = iter;
		res = matchFn(userkey, faux_list_data(node));
		if (!res)
			return node;
		if (res < 0) // No chances to find match
			return NULL;
	}

	return NULL;
}


/** @brief Search list for matching (match function) and returns user data.
 *
 * Same as faux_list_match_node() but returns user data structure.
 *
 * @sa faux_list_match_node()
 */
void *faux_list_match(const faux_list_t *list, faux_list_match_fn matchFn,
	const void *userkey, faux_list_node_t **saveptr) {

	faux_list_node_t *res =
		faux_list_match_node(list, matchFn, userkey, saveptr);
	if (!res)
		return NULL;
	return faux_list_data(res);
}


/** @brief Search list for first matching (match function).
 *
 * Same as faux_list_match_node() but search for the fisrt matching.
 * Doesn't use saveptr iterator.
 *
 * @sa faux_list_match_node()
 */
faux_list_node_t *faux_list_find_node(const faux_list_t *list,
	faux_list_match_fn matchFn, const void *userkey) {

	return faux_list_match_node(list, matchFn, userkey, NULL);
}


/** @brief Search list for first matching (match function) and returns user data.
 *
 * Same as faux_list_match_node() but returns user data structure and search
 * only for the first matching. Doesn't use saveptr iterator.
 *
 * @sa faux_list_match_node()
 */
void *faux_list_find(const faux_list_t *list, faux_list_match_fn matchFn,
	const void *userkey) {

	return faux_list_match(list, matchFn, userkey, NULL);
}
