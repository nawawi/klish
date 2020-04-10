/** @file str.c
 * @brief String related functions
 *
 * This file implements some often used string functions.
 * Some functions are more portable versions of standard
 * functions but others are original ones.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "faux/ctype.h"
#include "faux/str.h"

/* TODO: Are that vars really needed? */
//const char *lub_string_esc_default = "`|$<>&()#;\\\"!";
//const char *lub_string_esc_regex = "^$.*+[](){}";
//const char *lub_string_esc_quoted = "\\\"";


/** @brief Free the memory allocated for the string.
 *
 * Safely free the memory allocated for the string. You can use NULL
 * pointer with this function. POSIX's free() checks for the NULL pointer
 * but not all systems do so.
 *
 * @param [in] str String to free
 */
void faux_str_free(char *str) {

	faux_free(str);
}


/** @brief Duplicates the string.
 *
 * Duplicates the string. Same as standard strdup() function. Allocates
 * memory with malloc(). Checks for NULL pointer.
 *
 * @warning Resulting string must be freed by faux_str_free().
 *
 * @param [in] str String to duplicate.
 * @return Pointer to allocated string or NULL.
 */
char *faux_str_dup(const char *str) {

	if (!str)
		return NULL;
	return strdup(str);
}


/** @brief Duplicates the first n bytes of the string.
 *
 * Duplicates at most n bytes of the string. Allocates
 * memory with malloc(). Checks for NULL pointer. Function will allocate
 * n + 1 bytes to store string and terminating null byte.
 *
 * @warning Resulting string must be freed by faux_str_free().
 *
 * @param [in] str String to duplicate.
 * @param [in] n Number of bytes to copy.
 * @return Pointer to allocated string or NULL.
 */
char *faux_str_dupn(const char *str, size_t n) {

	char *res = NULL;
	size_t len = 0;

	if (!str)
		return NULL;
	len = strlen(str);
	len = (len < n) ? len : n;
	res = faux_zmalloc(len + 1);
	if (!res)
		return NULL;
	strncpy(res, str, len);
	res[len] = '\0';

	return res;
}


/** @brief Generates lowercase copy of input string.
 *
 * Allocates the copy of input string and convert that copy to lowercase.
 *
 * @warning Resulting string must be freed by faux_str_free().
 *
 * @param [in] str String to convert.
 * @return Pointer to lowercase string copy or NULL.
 */
char *faux_str_tolower(const char *str) {

	char *res = faux_str_dup(str);
	char *p = res;

	if (!res)
		return NULL;

	while (*p) {
		*p = faux_ctype_tolower(*p);
		p++;
	}

	return res;
}


/** @brief Generates uppercase copy of input string.
 *
 * Allocates the copy of input string and convert that copy to uppercase.
 *
 * @warning Resulting string must be freed by faux_str_free().
 *
 * @param [in] str String to convert.
 * @return Pointer to lowercase string copy or NULL.
 */
char *faux_str_toupper(const char *str) {

	char *res = faux_str_dup(str);
	char *p = res;

	if (!res)
		return NULL;

	while (*p) {
		*p = faux_ctype_toupper(*p);
		p++;
	}

	return res;
}


/** @brief Add n bytes of text to existent string.
 *
 * Concatenate two strings. Add n bytes of second string to the end of the
 * first one. The first argument is address of string pointer. The pointer
 * can be changed due to realloc() features. The first pointer can be NULL.
 * In this case the memory will be malloc()-ed and stored to the first pointer.
 *
 * @param [in,out] str Address of first string pointer.
 * @param [in] text Text to add to the first string.
 * @param [in] n Number of bytes to add.
 * @return Pointer to resulting string or NULL.
 */
char *faux_str_catn(char **str, const char *text, size_t n) {

	size_t str_len = 0;
	size_t text_len = 0;
	char *res = NULL;
	char *p = NULL;

	if (!text)
		return *str;

	str_len = (*str) ? strlen(*str) : 0;
	text_len = strlen(text);
	text_len = (text_len < n) ? text_len : n;

	res = realloc(*str, str_len + text_len + 1);
	if (!res)
		return NULL;
	p = res + str_len;
	strncpy(p, text, text_len);
	p[text_len] = '\0';
	*str = res;

	return res;
}


/** @brief Add some text to existent string.
 *
 * Concatenate two strings. Add second string to the end of the first one.
 * The first argument is address of string pointer. The pointer can be
 * changed due to realloc() features. The first pointer can be NULL. In this
 * case the memory will be malloc()-ed and stored to the first pointer.
 *
 * @param [in,out] str Address of first string pointer.
 * @param [in] text Text to add to the first string.
 * @return Pointer to resulting string or NULL.
 */
char *faux_str_cat(char **str, const char *text) {

	size_t len = 0;

	if (!text)
		return *str;
	len = strlen(text);

	return faux_str_catn(str, text, len);
}


/** @brief Service function to compare to chars in right way.
 *
 * The problem is char type can be signed or unsigned on different
 * platforms. So stright comparision can return different results.
 *
 * @param [in] char1 First char
 * @param [in] char2 Second char
 * @return
 * < 0 if char1 < char2
 * = 0 if char1 = char2
 * > 0 if char1 > char2
 */
static int faux_str_cmp_chars(char char1, char char2) {

	unsigned char ch1 = (unsigned char)char1;
	unsigned char ch2 = (unsigned char)char2;

	return (int)ch1 - (int)ch2;
}


/** @brief Compare n first characters of two strings ignoring case.
 *
 * The difference beetween this function an standard strncasecmp() is
 * faux function uses faux ctype functions. It can be important for
 * portability.
 *
 * @param [in] str1 First string to compare.
 * @param [in] str2 Second string to compare.
 * @param [in] n Number of characters to compare.
 * @return < 0, 0, > 0, see the strcasecmp().
 */
int faux_str_casecmpn(const char *str1, const char *str2, size_t n) {

	const char *p1 = str1;
	const char *p2 = str2;
	size_t num = n;

	while (*p1 != '\0' && *p2 != '\0' && num != 0) {
		int res = faux_str_cmp_chars(faux_ctype_tolower(*p1),
			faux_ctype_tolower(*p2));
		if (res != 0)
			return res;
		p1++;
		p2++;
		num--;
	}

	if (0 == n) // It means n first characters are equal.
		return 0;

	return faux_str_cmp_chars(faux_ctype_tolower(*p1),
		faux_ctype_tolower(*p2));
}


/** @brief Compare two strings ignoring case.
 *
 * The difference beetween this function an standard strcasecmp() is
 * faux function uses faux ctype functions. It can be important for
 * portability.
 *
 * @param [in] str1 First string to compare.
 * @param [in] str2 Second string to compare.
 * @return < 0, 0, > 0, see the strcasecmp().
 */
int faux_str_casecmp(const char *str1, const char *str2) {

	const char *p1 = str1;
	const char *p2 = str2;

	while (*p1 != '\0' && *p2 != '\0') {
		int res = faux_str_cmp_chars(faux_ctype_tolower(*p1),
			faux_ctype_tolower(*p2));
		if (res != 0)
			return res;
		p1++;
		p2++;
	}

	return faux_str_cmp_chars(faux_ctype_tolower(*p1),
		faux_ctype_tolower(*p2));
}


/** @brief Finds the first occurrence of the substring in the string
 *
 * Function is a faux version of strcasestr() function.
 *
 * @param [in] haystack String to find substring in it.
 * @param [in] needle Substring to find.
 * @return
 * Pointer to first occurence of substring in the string.
 * NULL on error
 */
char *faux_str_casestr(const char *haystack, const char *needle) {

	const char *ptr = haystack;
	size_t ptr_len = 0;
	size_t needle_len = 0;

	assert(haystack);
	assert(needle);
	if (!haystack || !needle)
		return NULL;

	ptr_len = strlen(haystack);
	needle_len = strlen(needle);

	while ((*ptr != '\0') && (ptr_len >= needle_len)) {
		int res = faux_str_casecmpn(ptr, needle, needle_len);
		if (0 == res)
			return (char *)ptr;
		ptr++;
		ptr_len--;
	}

	return NULL; // Not found
}


/** Prepare string for embedding to C-code (make escaping).
 *
 * @warning The returned pointer must be freed by faux_str_free().
 * @param [in] src String for escaping.
 * @return Escaped string or NULL on error.
 */
char *faux_str_c_esc(const char *src) {

	const char *src_ptr = src;
	char *dst = NULL;
	char *dst_ptr = NULL;
	char *escaped = NULL;
	size_t src_len = 0;
	size_t dst_len = 0;

	assert(src);
	if (!src)
		return NULL;

	src_len = strlen(src);
	// Calculate max destination string size.
	// The worst case is when each src character will be replaced by
	// something like '\xff'. So it's 4 dst chars for 1 src one.
	dst_len = (src_len * 4) + 1; // one byte for '\0'
	dst = faux_zmalloc(dst_len);
	assert(dst);
	if (!dst)
		return NULL;

	while (*src_ptr != '\0') {
		char *esc = NULL; // escaped replacement
		char buf[5]; // longest 'char' (4 bytes) + '\0'
		size_t len = 0;

		switch (*src_ptr) {
		case '\n':
			esc = "\\n";
			break;
		case '\"':
			esc = "\\\"";
			break;
		case '\\':
			esc = "\\\\";
			break;
		case '\'':
			esc = "\\\'";
			break;
		case '\r':
			esc = "\\r";
			break;
		case '\t':
			esc = "\\t";
			break;
		default:
			// Check is the symbol control character. Control
			// characters has codes from 0x00 to 0x1f.
			if (((unsigned char)*src_ptr & 0xe0) == 0) { // control
				snprintf(buf, sizeof(buf), "\\x%02x",
					(unsigned char)*src_ptr);
				buf[4] = '\0'; // for safety
			} else {
				buf[0] = *src_ptr; // Common character
				buf[1] = '\0';
			}
			esc = buf;
			break;
		}

		len = strlen(esc);
		memcpy(dst_ptr, esc, len); // zmalloc() nullify the rest
		dst_ptr += len;
		src_ptr++;
	}

	escaped = faux_str_dup(dst); // Free some memory
	faux_str_free(dst); // 'dst' size >= 'escaped' size

	return escaped;
}


/* TODO: If it nedeed?
const char *faux_str_nextword(const char *string,
	size_t *len, size_t *offset, size_t *quoted)
{
	const char *word;

	*quoted = 0;

	// Find the start of a word (not including an opening quote)
	while (*string && isspace(*string)) {
		string++;
		(*offset)++;
	}
	// Is this the start of a quoted string ?
	if (*string == '"') {
		*quoted = 1;
		string++;
	}
	word = string;
	*len = 0;

	// Find the end of the word
	while (*string) {
		if (*string == '\\') {
			string++;
			(*len)++;
			if (*string) {
				(*len)++;
				string++;
			}
			continue;
		}
		// End of word
		if (!*quoted && isspace(*string))
			break;
		if (*string == '"') {
			// End of a quoted string
			*quoted = 2;
			break;
		}
		(*len)++;
		string++;
	}

	return word;
}
*/

// TODO: Is it needed?
/*
char *lub_string_ndecode(const char *string, unsigned int len)
{
	const char *s = string;
	char *res, *p;
	int esc = 0;

	if (!string)
		return NULL;

	p = res = faux_zmalloc(len + 1);

	while (*s && (s < (string +len))) {
		if (!esc) {
			if ('\\' == *s)
				esc = 1;
			else
				*p = *s;
		} else {
//			switch (*s) {
//			case 'r':
//			case 'n':
//				*p = '\n';
//				break;
//			case 't':
//				*p = '\t';
//				break;
//			default:
//				*p = *s;
//				break;
//			}
//			*p = *s;
			esc = 0;
		}
		if (!esc)
			p++;
		s++;
	}
	*p = '\0';

	return res;
}
*/

// TODO: Is it needed?
/*
inline char *lub_string_decode(const char *string)
{
	return lub_string_ndecode(string, strlen(string));
}
*/

// TODO: Is it needed?
/*----------------------------------------------------------- */
/*
 * This needs to escape any dangerous characters within the command line
 * to prevent gaining access to the underlying system shell.
 */
/*
char *lub_string_encode(const char *string, const char *escape_chars)
{
	char *result = NULL;
	const char *p;

	if (!escape_chars)
		return lub_string_dup(string);
	if (string && !(*string)) // Empty string
		return lub_string_dup(string);

	for (p = string; p && *p; p++) {
		// find any special characters and prefix them with '\'
		size_t len = strcspn(p, escape_chars);
		lub_string_catn(&result, p, len);
		p += len;
		if (*p) {
			lub_string_catn(&result, "\\", 1);
			lub_string_catn(&result, p, 1);
		} else {
			break;
		}
	}
	return result;
}
*/



// TODO: Is it needed?
/*--------------------------------------------------------- */
/*
unsigned int lub_string_equal_part(const char *str1, const char *str2,
	bool_t utf8)
{
	unsigned int cnt = 0;

	if (!str1 || !str2)
		return cnt;
	while (*str1 && *str2) {
		if (*str1 != *str2)
			break;
		cnt++;
		str1++;
		str2++;
	}
	if (!utf8)
		return cnt;

	// UTF8 features
	if (cnt && (UTF8_11 == (*(str1 - 1) & UTF8_MASK)))
		cnt--;

	return cnt;
}
*/

// TODO: Is it needed?

/*--------------------------------------------------------- */
/*
const char *lub_string_suffix(const char *string)
{
	const char *p1, *p2;
	p1 = p2 = string;
	while (*p1) {
		if (faux_ctype_isspace(*p1)) {
			p2 = p1;
			p2++;
		}
		p1++;
	}
	return p2;
}
*/


// TODO: Is it needed?
/*--------------------------------------------------------- */
/*
unsigned int lub_string_wordcount(const char *line)
{
	const char *word;
	unsigned int result = 0;
	size_t len = 0, offset = 0;
	size_t quoted;

	for (word = lub_string_nextword(line, &len, &offset, &quoted);
		*word || quoted;
		word = lub_string_nextword(word + len, &len, &offset, &quoted)) {
		// account for the terminating quotation mark
		len += quoted ? quoted - 1 : 0;
		result++;
	}

	return result;
}
*/
