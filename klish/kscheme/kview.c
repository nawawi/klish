#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <klish/kview.h>


struct kview_s {
	bool_t is_static;
	kview_info_t info;
};


static kview_t *kview_new_internal(kview_info_t info, bool_t is_static)
{
	kview_t *view = NULL;

	view = faux_zmalloc(sizeof(*view));
	assert(view);
	if (!view)
		return NULL;

	// Initialize
	view->is_static = is_static;
	view->info = info;

	return view;
}


kview_t *kview_new(kview_info_t info)
{
	return kview_new_internal(info, BOOL_FALSE);
}


kview_t *kview_new_static(kview_info_t info)
{
	return kview_new_internal(info, BOOL_TRUE);
}


void kview_free(kview_t *view)
{
	if (!view)
		return;

	if (!view->is_static) {
		faux_str_free(view->info.name);
	}

	faux_free(view);
}


const char *kview_name(const kview_t *view)
{
	assert(view);
	if (!view)
		return NULL;

	return view->info.name;
}
