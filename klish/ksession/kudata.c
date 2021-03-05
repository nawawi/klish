/** @file kudata.c
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <faux/str.h>
#include <klish/khelper.h>
#include <klish/kudata.h>

struct kudata_s {
	char *name;
	void *data;
	kudata_data_free_fn free_fn;
};

// Name
KGET_STR(udata, name);

// Data
KGET(udata, void *, data);
KSET(udata, void *, data);

// Data
KGET(udata, kudata_data_free_fn, free_fn);
KSET(udata, kudata_data_free_fn, free_fn);


kudata_t *kudata_new(const char *name)
{
	kudata_t *udata = NULL;

	if (faux_str_is_empty(name))
		return NULL;

	udata = faux_zmalloc(sizeof(*udata));
	assert(udata);
	if (!udata)
		return NULL;

	// Initialize
	udata->name = faux_str_dup(name);
	udata->data = NULL;
	udata->free_fn = NULL;

	return udata;
}


void kudata_free(kudata_t *udata)
{
	if (!udata)
		return;

	faux_str_free(udata->name);
	if (udata->free_fn)
		udata->free_fn(udata->data);

	faux_free(udata);
}
