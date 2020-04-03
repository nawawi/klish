/*
 * conv.c
 */
#include "private.h"

#include <stdlib.h>
#include <errno.h>
#include <limits.h>

/*--------------------------------------------------------- */
int lub_conv_atol(const char *str, long int *val, int base)
{
	char *endptr;
	long int tmp;

	errno = 0;
	tmp = strtol(str, &endptr, base);
	if ((errno != 0) || (endptr == str))
		return -1;
	*val = tmp;
	return 0;
}

/*--------------------------------------------------------- */
int lub_conv_atoul(const char *str, unsigned long int *val, int base)
{
	long int tmp;
	if (lub_conv_atol(str, &tmp, base) < 0)
		return -1;
	if ((tmp < 0) || (tmp > LONG_MAX)) /* Overflow */
		return -1;
	*val = tmp;
	return 0;
}

/*--------------------------------------------------------- */
int lub_conv_atos(const char *str, short *val, int base)
{
	long int tmp;
	if (lub_conv_atol(str, &tmp, base) < 0)
		return -1;
	if ((tmp < SHRT_MIN) || (tmp > SHRT_MAX)) /* Overflow */
		return -1;
	*val = tmp;
	return 0;
}

/*--------------------------------------------------------- */
int lub_conv_atous(const char *str, unsigned short *val, int base)
{
	unsigned long int tmp;
	if (lub_conv_atoul(str, &tmp, base) < 0)
		return -1;
	if (tmp > USHRT_MAX) /* Overflow */
		return -1;
	*val = tmp;
	return 0;
}

/*--------------------------------------------------------- */
int lub_conv_atoi(const char *str, int *val, int base)
{
	long int tmp;
	if (lub_conv_atol(str, &tmp, base) < 0)
		return -1;
	if ((tmp < INT_MIN) || (tmp > INT_MAX)) /* Overflow */
		return -1;
	*val = tmp;
	return 0;
}

/*--------------------------------------------------------- */
int lub_conv_atoui(const char *str, unsigned int *val, int base)
{
	unsigned long int tmp;
	if (lub_conv_atoul(str, &tmp, base) < 0)
		return -1;
	if (tmp > UINT_MAX) /* Overflow */
		return -1;
	*val = tmp;
	return 0;
}
