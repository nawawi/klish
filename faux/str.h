/** @file str.h
 * @brief Public interface for faux string functions.
 */

#ifndef _faux_str_h
#define _faux_str_h

#include <stddef.h>

#include "faux/faux.h"

#define UTF8_MASK 0xC0
#define UTF8_7BIT_MASK 0x80 // One byte or multibyte
#define UTF8_11   0xC0 // First UTF8 byte
#define UTF8_10   0x80 // Next UTF8 bytes

C_DECL_BEGIN

void faux_str_free(char *str);

char *faux_str_dupn(const char *str, size_t n);
char *faux_str_dup(const char *str);

char *faux_str_catn(char **str, const char *text, size_t n);
char *faux_str_cat(char **str, const char *text);

char *faux_str_tolower(const char *str);
char *faux_str_toupper(const char *str);

int faux_str_ncasecmp(const char *str1, const char *str2, size_t n);
int faux_str_casecmp(const char *str1, const char *str2);


//const char *faux_str_suffix(const char *string);
//const char *faux_str_nocasestr(const char *cs, const char *ct);
/*
 * These are the escape characters which are used by default when 
 * expanding variables. These characters will be backslash escaped
 * to prevent them from being interpreted in a script.
 *
 * This is a security feature to prevent users from arbitarily setting
 * parameters to contain special sequences.
 */
//extern const char *faux_str_esc_default;
//extern const char *faux_str_esc_regex;
//extern const char *faux_str_esc_quoted;

//char *faux_str_decode(const char *string);
//char *faux_str_ndecode(const char *string, unsigned int len);
//char *faux_str_encode(const char *string, const char *escape_chars);
//unsigned int faux_str_equal_part(const char *str1, const char *str2,
//	bool_t utf8);
//const char *faux_str_nextword(const char *string,
//	size_t *len, size_t *offset, size_t *quoted);
//unsigned int faux_str_wordcount(const char *line);

C_DECL_END

#endif				/* _faux_str_h */
