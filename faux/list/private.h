#include "faux/list.h"

struct faux_list_node_s {
	faux_list_node_t *prev;
	faux_list_node_t *next;
	void *data;
};

struct faux_list_s {
	faux_list_node_t *head;
	faux_list_node_t *tail;
	faux_list_compare_fn *compareFn; // Function to compare two list elements
	faux_list_free_fn *freeFn; // Function to properly free data field
	size_t len;
};
