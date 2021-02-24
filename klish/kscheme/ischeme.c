#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <faux/list.h>
#include <faux/error.h>
#include <klish/kview.h>
#include <klish/kscheme.h>


bool_t kscheme_nested_from_ischeme(kscheme_t *kscheme, ischeme_t *ischeme,
	faux_error_t *error_stack)
{
	if (!kscheme || !ischeme) {
		if (error_stack)
			faux_error_add(error_stack,
				kscheme_strerror(KSCHEME_ERROR_INTERNAL));
		return BOOL_FALSE;
	}

	// View list
	if (ischeme->views) {
		iview_t **p_iview = NULL;
		for (p_iview = *ischeme->views; *p_iview; p_iview++) {
			kview_t *kview = NULL;
			iview_t *iview = *p_iview;
			kview_error_e kview_error = KVIEW_ERROR_OK;

			kview = kview_new(iview, &kview_error);
			if (!kview) {
				if (error_stack)
					faux_error_add(error_stack,
						kview_strerror(kview_error));
				continue;
			}
//			kview_from_iview
			printf("view %p %s\n", iview, iview->name);
		}
	}

	return BOOL_TRUE;
}


kscheme_t *kscheme_from_ischeme(ischeme_t *ischeme, faux_error_t *error_stack)
{
	kscheme_t *kscheme = NULL;
	kscheme_error_e kscheme_error = KSCHEME_ERROR_OK;

	kscheme = kscheme_new(&kscheme_error);
	if (!kscheme) {
		if (error_stack)
			faux_error_add(error_stack,
				kscheme_strerror(kscheme_error));
		return NULL;
	}

	kscheme_nested_from_ischeme(kscheme, ischeme, error_stack);

	return kscheme;
}