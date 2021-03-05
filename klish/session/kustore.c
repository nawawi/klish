/** @file kudata.c
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


struct kudata_s {
	char *name;
	void *data;
};


// User data blobs list
KCMP_NESTED(udata, plugin, name);
KCMP_NESTED_BY_KEY(udata, plugin, name);
KADD_NESTED(udata, plugin);
KFIND_NESTED(udata, plugin);

// PTYPE list
KCMP_NESTED(udata, ptype, name);
KCMP_NESTED_BY_KEY(udata, ptype, name);
KADD_NESTED(udata, ptype);
KFIND_NESTED(udata, ptype);

// VIEW list
KCMP_NESTED(udata, view, name);
KCMP_NESTED_BY_KEY(udata, view, name);
KADD_NESTED(udata, view);
KFIND_NESTED(udata, view);


kudata_t *kudata_new(kudata_error_e *error)
{
	kudata_t *udata = NULL;

	udata = faux_zmalloc(sizeof(*udata));
	assert(udata);
	if (!udata) {
		if (error)
			*error = KSCHEME_ERROR_ALLOC;
		return NULL;
	}

	// PLUGIN list
	udata->plugins = faux_list_new(FAUX_LIST_SORTED, FAUX_LIST_UNIQUE,
		kudata_plugin_compare, kudata_plugin_kcompare,
		(void (*)(void *))kplugin_free);
	assert(udata->plugins);

	// PTYPE list
	udata->ptypes = faux_list_new(FAUX_LIST_SORTED, FAUX_LIST_UNIQUE,
		kudata_ptype_compare, kudata_ptype_kcompare,
		(void (*)(void *))kptype_free);
	assert(udata->ptypes);

	// VIEW list
	udata->views = faux_list_new(FAUX_LIST_SORTED, FAUX_LIST_UNIQUE,
		kudata_view_compare, kudata_view_kcompare,
		(void (*)(void *))kview_free);
	assert(udata->views);

	return udata;
}



/*--------------------------------------------------------- */
int kudata_compare(const void *first, const void *second)
{
	const kudata_t *f = (const kudata_t *)first;
	const kudata_t *s = (const kudata_t *)second;

	return strcmp(f->name, s->name);
}

/*--------------------------------------------------------- */
kudata_t *kudata_new(const char *name, void *data)
{
	kudata_t *pdata =
		(kudata_t *)malloc(sizeof(kudata_t));

	if (!name) {
		free(pdata);
		return NULL;
	}
	memset(pdata, 0, sizeof(*pdata));
	pdata->name = lub_string_dup(name);
	pdata->data = data;

	return pdata;
}

/*--------------------------------------------------------- */
void *kudata_free(kudata_t *this)
{
	void *data;

	if (!this)
		return NULL;
	if (this->name)
		lub_string_free(this->name);
	data = this->data;
	free(this);

	return data;
}

/*--------------------------------------------------------- */
void kudata_delete(void *data)
{
	kudata_t *this = (kudata_t *)data;
	kudata_free(this);
}
