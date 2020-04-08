#include "faux/list.h"

struct faux_pair_s {
	char *name;
	char *value;
};

struct faux_ini_s {
	faux_list_t *list;
};
