#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <faux/list.h>
#include <klish/kview.h>
#include <klish/kscheme.h>


kscheme_t *kscheme_from_ischeme(kscheme_t *kscheme, ischeme_t *ischeme, faux_list_t *error_stack)
{
//	iview_t *iview = NULL;
	kview_t *kview = NULL;

	if (!kscheme)
		return NULL;

	if (ischeme->views) {
		iview_t **piview = NULL;
		for (piview = *ischeme->views; *piview; piview++) {
			printf("view %p %s\n", *piview, (*piview)->name);
		}
		kview = kview;
	}

	error_stack = error_stack;
	return NULL;
}