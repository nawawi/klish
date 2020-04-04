/** @file conv.c
 * @brief Functions to convert from string to integer.
 */

#include <stdlib.h>
#include <errno.h>
#include <limits.h>


/** @brief Converts string to long int
 *
 * Converts string to long int and check for overflow and valid
 * input values. Function indicates error by return value. It
 * returns the convertion result by second argument.
 *
 * @param [in] str Input string to convert.
 * @param [out] val Pointer to result value.
 * @param [in] base Base to convert.
 * @return 0 - success, < 0 - error
 */
int faux_conv_atol(const char *str, long int *val, int base) {

	char *endptr;
	long int res;

	res = strtol(str, &endptr, base);
	// Check fof overflow
	if (((LONG_MIN == res) || (LONG_MAX == res)) && (ERANGE == errno))
		return -1;
	// No valid digits at all
	if ((0 == res) && ((endptr == str) || (errno != 0)))
		return -1;
	*val = res;

	return 0;
}


/** @brief Converts string to unsigned long int
 *
 * Converts string to unsigned long int and check for overflow and valid
 * input values. Function indicates error by return value. It
 * returns the convertion result by second argument.
 *
 * @param [in] str Input string to convert.
 * @param [out] val Pointer to result value.
 * @param [in] base Base to convert.
 * @return 0 - success, < 0 - error
 */
int faux_conv_atoul(const char *str, unsigned long int *val, int base) {

	char *endptr;
	unsigned long int res;

	res = strtoul(str, &endptr, base);
	// Check fof overflow
	if ((ULONG_MAX == res) && (ERANGE == errno))
		return -1;
	// No valid digits at all
	if ((0 == res) && ((endptr == str) || (errno != 0)))
		return -1;
	*val = res;

	return 0;
}


/*--------------------------------------------------------- */
int faux_conv_atos(const char *str, short *val, int base)
{
	long int tmp;
	if (faux_conv_atol(str, &tmp, base) < 0)
		return -1;
	if ((tmp < SHRT_MIN) || (tmp > SHRT_MAX)) /* Overflow */
		return -1;
	*val = tmp;
	return 0;
}

/*--------------------------------------------------------- */
int faux_conv_atous(const char *str, unsigned short *val, int base)
{
	unsigned long int tmp;
	if (faux_conv_atoul(str, &tmp, base) < 0)
		return -1;
	if (tmp > USHRT_MAX) /* Overflow */
		return -1;
	*val = tmp;
	return 0;
}

/*--------------------------------------------------------- */
int faux_conv_atoi(const char *str, int *val, int base)
{
	long int tmp;
	if (faux_conv_atol(str, &tmp, base) < 0)
		return -1;
	if ((tmp < INT_MIN) || (tmp > INT_MAX)) /* Overflow */
		return -1;
	*val = tmp;
	return 0;
}

/*--------------------------------------------------------- */
int faux_conv_atoui(const char *str, unsigned int *val, int base)
{
	unsigned long int tmp;
	if (faux_conv_atoul(str, &tmp, base) < 0)
		return -1;
	if (tmp > UINT_MAX) /* Overflow */
		return -1;
	*val = tmp;
	return 0;
}
