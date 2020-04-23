#include "faux/list.h"

struct faux_list_node_s {
	faux_list_node_t *prev;
	faux_list_node_t *next;
	void *data;
};

struct faux_list_s {
	faux_list_node_t *head;
	faux_list_node_t *tail;
	bool_t sorted;
	bool_t unique;
	faux_list_cmp_fn cmpFn; // Function to compare two list elements
	faux_list_kcmp_fn kcmpFn; // Function to compare key and list element
	faux_list_free_fn freeFn; // Function to properly free data field
	size_t len;
};
