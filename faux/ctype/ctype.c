/*
 * ctype.c
 */
#include "lub/ctype.h"
#include <ctype.h>

/*--------------------------------------------------------- */
bool_t lub_ctype_isdigit(char c)
{
	unsigned char tmp = (unsigned char)c;
	return isdigit(tmp) ? BOOL_TRUE : BOOL_FALSE;
}

/*--------------------------------------------------------- */
bool_t lub_ctype_isspace(char c)
{
	unsigned char tmp = (unsigned char)c;
	return isspace(tmp) ? BOOL_TRUE : BOOL_FALSE;
}

/*--------------------------------------------------------- */
char lub_ctype_tolower(char c)
{
	unsigned char tmp = (unsigned char)c;
	return tolower(tmp);
}

/*--------------------------------------------------------- */
char lub_ctype_toupper(char c)
{
	unsigned char tmp = (unsigned char)c;
	return toupper(tmp);
}

/*--------------------------------------------------------- */
