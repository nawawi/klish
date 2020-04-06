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

	node = malloc(sizeof(*node));
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

	if (node)
		free(node);
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
 * @param [in] compareFn Callback function to compare two user data instances
 * to sort list.
 * @param [in] freeFn Callback function to free user data.
 * @return Newly created bidirectional list or NULL on error.
 */
faux_list_t *faux_list_new(faux_list_compare_fn compareFn,
	faux_list_free_fn freeFn) {

	faux_list_t *list;

	list = malloc(sizeof(*list));
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
	free(list);
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

/*--------------------------------------------------------- */
faux_list_node_t *faux_list_add(faux_list_t *this, void *data) {
	return faux_list_add_generic(this, data, BOOL_FALSE, BOOL_FALSE);
}

/*--------------------------------------------------------- */
faux_list_node_t *faux_list_add_uniq(faux_list_t *this, void *data) {
	return faux_list_add_generic(this, data, BOOL_TRUE, BOOL_FALSE);
}

/*--------------------------------------------------------- */
faux_list_node_t *faux_list_find_add(faux_list_t *this, void *data) {
	return faux_list_add_generic(this, data, BOOL_TRUE, BOOL_TRUE);
}


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

/*--------------------------------------------------------- */
void faux_list_del(faux_list_t *list, faux_list_node_t *node) {

	void *data = NULL;

	assert(list);
	assert(node);
	if (!list || !node)
		return;

	data = faux_list_takeaway(list, node);
	if (list->freeFn)
		list->freeFn(data);
}


/*--------------------------------------------------------- */
faux_list_node_t *faux_list_match_node(faux_list_t *this,
	faux_list_match_fn matchFn, const void *userkey,
	faux_list_node_t **saveptr) {
	faux_list_node_t *iter = NULL;

	if (!this || !matchFn || !this->head)
		return NULL;
	if (saveptr)
		iter = *saveptr;
	if (!iter)
		iter = this->head;
	while (iter) {
		int res;
		faux_list_node_t *node = iter;

		iter = faux_list_next_node(iter);
		if (saveptr)
			*saveptr = iter;
		res = matchFn(userkey, faux_list_data(node));
		if (!res)
			return node;
		if (res < 0)	// No chances to find match
			return NULL;
	}

	return NULL;
}

/*--------------------------------------------------------- */
faux_list_node_t *faux_list_find_node(faux_list_t *this,
	faux_list_match_fn matchFn, const void *userkey) {
	return faux_list_match_node(this, matchFn, userkey, NULL);
}

/*--------------------------------------------------------- */
void *faux_list_match(faux_list_t *this,
	faux_list_match_fn matchFn, const void *userkey,
	faux_list_node_t **saveptr) {
	faux_list_node_t *res =
		faux_list_match_node(this, matchFn, userkey, saveptr);
	if (!res)
		return NULL;
	return faux_list_data(res);
}

/*--------------------------------------------------------- */
void *faux_list_find(faux_list_t *this,
	faux_list_match_fn matchFn, const void *userkey) {
	return faux_list_match(this, matchFn, userkey, NULL);
}
