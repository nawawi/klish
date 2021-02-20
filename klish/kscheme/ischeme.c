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

	if (!kscheme)
		return NULL;

	if (ischeme->views) {
		iview_t **p_iview = NULL;
		for (p_iview = *ischeme->views; *p_iview; p_iview++) {
			kview_t *kview = NULL;
			iview_t *iview = *p_iview;
			printf("view %p %s\n", iview, iview->name);
			kview = kview;
		}
	}

	error_stack = error_stack;
	return NULL;
}