/*
 *
 */

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <klish/kcontext.h>


int klish_nop(kcontext_t *context)
{
	context = context; // Happy compiler

	return 0;
}
