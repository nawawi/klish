/** @file kustore.c
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <faux/list.h>
#include <klish/khelper.h>
#include <klish/kudata.h>
#include <klish/kustore.h>

struct kustore_s {
	faux_list_t *udatas;
};


// User data blobs list
KCMP_NESTED(ustore, udata, name);
KCMP_NESTED_BY_KEY(ustore, udata, name);
KADD_NESTED(ustore, udata);
KFIND_NESTED(ustore, udata);


kustore_t *kustore_new()
{
	kustore_t *ustore = NULL;

	ustore = faux_zmalloc(sizeof(*ustore));
	assert(ustore);
	if (!ustore)
		return NULL;

	// User data blobs list
	ustore->udatas = faux_list_new(FAUX_LIST_SORTED, FAUX_LIST_UNIQUE,
		kustore_udata_compare, kustore_udata_kcompare,
		(void (*)(void *))kudata_free);
	assert(ustore->udatas);

	return ustore;
}


void kustore_free(kustore_t *ustore)
{
	if (!ustore)
		return;

	faux_list_free(ustore->udatas);

	free(ustore);
}


kudata_t *kustore_slot_new(kustore_t *ustore,
	const char *name, void *data, kudata_data_free_fn free_fn)
{
	kudata_t *udata = NULL;

	assert(ustore);
	if (!ustore)
		return NULL;

	if (!name)
		return NULL;

	udata = kudata_new(name);
	if (!udata)
		return NULL;

	kudata_set_data(udata, data);
	kudata_set_free_fn(udata, free_fn);

	return udata;
}


void *kustore_slot_data(kustore_t *ustore, const char *udata_name)
{
	kudata_t *udata = NULL;

	assert(ustore);
	if (!ustore)
		return NULL;

	if (!udata_name)
		return NULL;

	udata = kustore_find_udata(ustore, udata_name);
	if (!udata)
		return NULL;

	return kudata_data(udata);
}
