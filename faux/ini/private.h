#include "faux/faux.h"
#include "faux/list.h"
#include "faux/ini.h"

struct faux_pair_s {
	char *name;
	char *value;
};

struct faux_ini_s {
	faux_list_t *list;
};

C_DECL_BEGIN

int faux_pair_compare(const void *first, const void *second);
int faux_pair_kcompare(const void *key, const void *list_item);
faux_pair_t *faux_pair_new(const char *name, const char *value);
void faux_pair_free(void *pair);

void faux_pair_set_name(faux_pair_t *pair, const char *name);
void faux_pair_set_value(faux_pair_t *pair, const char *value);

C_DECL_END
