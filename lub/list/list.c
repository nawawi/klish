#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "private.h"

/*--------------------------------------------------------- */
static void lub_list_init(lub_list_t *this,
	lub_list_compare_fn compareFn,
	lub_list_free_fn freeFn)
{
	this->head = NULL;
	this->tail = NULL;
	this->compareFn = compareFn;
	this->freeFn = freeFn;
	this->len = 0;
}

/*--------------------------------------------------------- */
lub_list_t *lub_list_new(lub_list_compare_fn compareFn,
	lub_list_free_fn freeFn)
{
	lub_list_t *this;

	this = malloc(sizeof(*this));
	assert(this);
	lub_list_init(this, compareFn, freeFn);

	return this;
}

/*--------------------------------------------------------- */
inline void lub_list_free(lub_list_t *this)
{
	free(this);
}

/*--------------------------------------------------------- */
/* Free all nodes and data from list and finally
 * free the list itself. It uses special callback
 * function specified by user to free the abstract
 * data.
 */
void lub_list_free_all(lub_list_t *this)
{
	lub_list_node_t *iter;

	while ((iter = lub_list__get_head(this))) {
		lub_list_del(this, iter);
		if (this->freeFn)
			this->freeFn(lub_list_node__get_data(iter));
		lub_list_node_free(iter);
	}
	lub_list_free(this);
}

/*--------------------------------------------------------- */
inline lub_list_node_t *lub_list__get_head(lub_list_t *this)
{
	return this->head;
}

/*--------------------------------------------------------- */
inline lub_list_node_t *lub_list__get_tail(lub_list_t *this)
{
	return this->tail;
}

/*--------------------------------------------------------- */
static void lub_list_node_init(lub_list_node_t *this,
	void *data)
{
	this->prev = this->next = NULL;
	this->data = data;
}

/*--------------------------------------------------------- */
lub_list_node_t *lub_list_node_new(void *data)
{
	lub_list_node_t *this;

	this = malloc(sizeof(*this));
	assert(this);
	lub_list_node_init(this, data);

	return this;
}

/*--------------------------------------------------------- */
inline lub_list_node_t *lub_list_iterator_init(lub_list_t *this)
{
	return this->head;
}

/*--------------------------------------------------------- */
inline lub_list_node_t *lub_list_node__get_prev(lub_list_node_t *this)
{
	return this->prev;
}

/*--------------------------------------------------------- */
inline lub_list_node_t *lub_list_node__get_next(lub_list_node_t *this)
{
	return this->next;
}

/*--------------------------------------------------------- */
inline lub_list_node_t *lub_list_iterator_next(lub_list_node_t *this)
{
	return lub_list_node__get_next(this);
}

/*--------------------------------------------------------- */
inline lub_list_node_t *lub_list_iterator_prev(lub_list_node_t *this)
{
	return lub_list_node__get_prev(this);
}

/*--------------------------------------------------------- */
inline void lub_list_node_free(lub_list_node_t *this)
{
	free(this);
}

/*--------------------------------------------------------- */
inline void *lub_list_node__get_data(lub_list_node_t *this)
{
	return this->data;
}

/*--------------------------------------------------------- */
/* uniq - true/false Don't add entry with identical order
 *	key (when the compareFn() returns 0)
 * find - true/false Function returns list_node if there is
 *	identical entry. Or NULL if find is false.
 */
static lub_list_node_t *lub_list_add_generic(lub_list_t *this, void *data,
	bool_t uniq, bool_t find)
{
	lub_list_node_t *node = lub_list_node_new(data);
	lub_list_node_t *iter;

	this->len++;

	/* Empty list */
	if (!this->head) {
		this->head = node;
		this->tail = node;
		return node;
	}

	/* Not sorted list. Add to the tail. */
	if (!this->compareFn) {
		node->prev = this->tail;
		node->next = NULL;
		this->tail->next = node;
		this->tail = node;
		return node;
	}

	/* Sorted list */
	iter = this->tail;
	while (iter) {
		int res = this->compareFn(node->data, iter->data);
		if (uniq && (res == 0)) {
			this->len--; // Revert previous increment
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
	/* Insert node into the list head */
	if (!iter) {
		node->next = this->head;
		node->prev = NULL;
		this->head->prev = node;
		this->head = node;
	}
	if (!node->next)
		this->tail = node;

	return node;
}

/*--------------------------------------------------------- */
lub_list_node_t *lub_list_add(lub_list_t *this, void *data)
{
	return lub_list_add_generic(this, data, BOOL_FALSE, BOOL_FALSE);
}

/*--------------------------------------------------------- */
lub_list_node_t *lub_list_add_uniq(lub_list_t *this, void *data)
{
	return lub_list_add_generic(this, data, BOOL_TRUE, BOOL_FALSE);
}

/*--------------------------------------------------------- */
lub_list_node_t *lub_list_find_add(lub_list_t *this, void *data)
{
	return lub_list_add_generic(this, data, BOOL_TRUE, BOOL_TRUE);
}

/*--------------------------------------------------------- */
void lub_list_del(lub_list_t *this, lub_list_node_t *node)
{
	if (node->prev)
		node->prev->next = node->next;
	else
		this->head = node->next;
	if (node->next)
		node->next->prev = node->prev;
	else
		this->tail = node->prev;

	this->len--;
}

/*--------------------------------------------------------- */
inline void lub_list_node_copy(lub_list_node_t *dst, lub_list_node_t *src)
{
	memcpy(dst, src, sizeof(lub_list_node_t));
}

/*--------------------------------------------------------- */
lub_list_node_t *lub_list_match_node(lub_list_t *this,
	lub_list_match_fn matchFn, const void *userkey,
	lub_list_node_t **saveptr)
{
	lub_list_node_t *iter = NULL;
	if (!this || !matchFn || !this->head)
		return NULL;
	if (saveptr)
		iter = *saveptr;
	if (!iter)
		iter = this->head;
	while (iter) {
		int res;
		lub_list_node_t *node = iter;
		iter = lub_list_node__get_next(iter);
		if (saveptr)
			*saveptr = iter;
		res = matchFn(userkey, lub_list_node__get_data(node));
		if (!res)
			return node;
		if (res < 0) // No chances to find match
			return NULL;
	}

	return NULL;
}

/*--------------------------------------------------------- */
void *lub_list_find_node(lub_list_t *this,
	lub_list_match_fn matchFn, const void *userkey)
{
	return lub_list_match_node(this, matchFn, userkey, NULL);
}

/*--------------------------------------------------------- */
void *lub_list_match(lub_list_t *this,
	lub_list_match_fn matchFn, const void *userkey,
	lub_list_node_t **saveptr)
{
	lub_list_node_t *res = lub_list_match_node(this, matchFn, userkey, saveptr);
	if (!res)
		return NULL;
	return lub_list_node__get_data(res);
}

/*--------------------------------------------------------- */
void *lub_list_find(lub_list_t *this,
	lub_list_match_fn matchFn, const void *userkey)
{
	return lub_list_match(this, matchFn, userkey, NULL);
}

/*--------------------------------------------------------- */
inline unsigned int lub_list_len(lub_list_t *this)
{
	return this->len;
}
