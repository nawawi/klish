/** @file ctype.c
 * @brief The ctype functions
 *
 * Some ctype functions are not compatible among different OSes.
 * So faux library functions use their own versions of some
 * ctype functions to unify the behaviour. Really for now all
 * faux ctype functions are only a wrappers for standard functions.
 * But it can be changed in future for portability purposes.
 */

#include <ctype.h>

#include "faux/ctype.h"

/** @brief Checks for a digit
 *
 * The function is same as standard isdigit() but gets char type
 * as an argument.
 *
 * @param [in] c Character to classify.
 * @return BOOL_TRUE if char is digit and BOOL_FALSE else.
 */
bool_t faux_ctype_isdigit(char c) {

	// isdigit() man says that argument must be unsigned char
	return isdigit((unsigned char)c) ? BOOL_TRUE : BOOL_FALSE;
}


/** @brief Checks for a white space
 *
 * The function is same as standard isspace() but gets char type
 * as an argument.
 *
 * @param [in] c Character to classify.
 * @return BOOL_TRUE if char is space and BOOL_FALSE else.
 */
bool_t faux_ctype_isspace(char c) {

	// isspace() man says that argument must be unsigned char
	return isspace((unsigned char)c) ? BOOL_TRUE : BOOL_FALSE;
}


/** @brief Converts uppercase characters to lowercase
 *
 * The function is same as standard tolower() but gets char type
 * as an argument.
 *
 * @param [in] c Character to convert.
 * @return Converted character.
 */
char faux_ctype_tolower(char c) {

	// tolower() man says that argument must be unsigned char
	return tolower((unsigned char)c);
}

/** @brief Converts lowercase characters to uppercase
 *
 * The function is same as standard toupper() but gets char type
 * as an argument.
 *
 * @param [in] c Character to convert.
 * @return Converted character.
 */
char faux_ctype_toupper(char c) {

	// toupper() man says that argument must be unsigned char
	return toupper((unsigned char)c);
}
