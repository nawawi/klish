/*
 * string_dup.c
 */
#include "private.h"

#include <string.h>
/*--------------------------------------------------------- */
char *
lub_string_dup(const char *string)
{
	char *result = NULL;
	if(NULL != string)
	{
		lub_string_cat(&result,string);
	}
	return result;
}
/*--------------------------------------------------------- */
