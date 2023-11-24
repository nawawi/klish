/*
 * tinyrl.c
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>

#include <faux/str.h>

#include "private.h"


/** @brief Converts UTF-8 char to unsigned long wchar
 *
 * @param [in] sp Pointer to UTF-8 string to get char from.
 * @param [out] sym_out Resulting wchar.
 * @return Number of bytes for current UTF-8 symbol.
 */
ssize_t utf8_to_wchar(const char *sp, unsigned long *sym_out)
{
	int i = 0;
	int octets = 0; // Number of 0x10xxxxxx UTF-8 sequence bytes
	unsigned long sym = 0;
	const unsigned char *p = (const unsigned char *)sp;

	if (sym_out)
		*sym_out = *p;

	if (!*p)
		return 0;

	// Check for first byte of UTF-8
	if ((*p & 0x80) == 0) // 0xxxxxxx
		return 1;

	// Analyze first byte to get number of UTF-8 octets
	if ((*p & 0xe0) == 0xc0) { // 110xxxxx 10xxxxxx
		octets = 1;
		sym = (*p & 0x1f);
	} else if ((*p & 0xf0) == 0xe0) { // 1110xxxx 10xxxxxx 10xxxxxx
		octets = 2;
		sym = (*p & 0xf);
	} else if ((*p & 0xf8) == 0xf0) { // 11110xxx 10xxxxxx 10xxxxxx
		octets = 3;
		sym = (*p & 7);
	} else if ((*p & 0xfc) == 0xf8) { // depricated
		octets = 4;
		sym = (*p & 3);
	} else if ((*p & 0xfe) == 0xfc) { // depricated
		octets = 5;
		sym = (*p & 1);
	} else {
		return 1; // Error but be robust and skip one byte
	}
	p++;

	// Analyze next UTF-8 bytes 10xxxxxx
	for (i = 0; i < octets; i++) {
		sym <<= 6;
		// Check if it's really UTF-8 bytes
		if ((*p & 0xc0) != 0x80)
			return 1; // Skip one byte if broken UTF-8 symbol
		sym |= (*p & 0x3f);
		p++;
	}

	if (sym_out)
		*sym_out = sym;

	return (octets + 1);
}


/** @brief Checks is wchar CJK
 *
 * @param [in] sym Widechar symbol to analyze
 * @return BOOL_TRUE if CJK and BOOL_FALSE else
 */
bool_t utf8_wchar_is_cjk(unsigned long sym)
{
	if (sym < 0x1100) /* Speed up for non-CJK chars */
		return BOOL_FALSE;

	if (sym >= 0x1100 && sym <= 0x11FF) /* Hangul Jamo */
		return BOOL_TRUE;
#if 0
	if (sym >=0x2E80 && sym <= 0x2EFF) /* CJK Radicals Supplement */
		return BOOL_TRUE;
	if (sym >=0x2F00 && sym <= 0x2FDF) /* Kangxi Radicals */
		return BOOL_TRUE;
	if (sym >= 0x2FF0 && sym <= 0x2FFF) /* Ideographic Description Characters */
		return BOOL_TRUE;
	if (sym >= 0x3000 && sym < 0x303F) /* CJK Symbols and Punctuation. The U+303f is half space */
		return BOOL_TRUE;
	if (sym >= 0x3040 && sym <= 0x309F) /* Hiragana */
		return BOOL_TRUE;
	if (sym >= 0x30A0 && sym <=0x30FF) /* Katakana */
		return BOOL_TRUE;
	if (sym >= 0x3100 && sym <=0x312F) /* Bopomofo */
		return BOOL_TRUE;
	if (sym >= 0x3130 && sym <= 0x318F) /* Hangul Compatibility Jamo */
		return BOOL_TRUE;
	if (sym >= 0x3190 && sym <= 0x319F) /* Kanbun */
		return BOOL_TRUE;
	if (sym >= 0x31A0 && sym <= 0x31BF) /* Bopomofo Extended */
		return BOOL_TRUE;
	if (sym >= 0x31C0 && sym <= 0x31EF) /* CJK strokes */
		return BOOL_TRUE;
	if (sym >= 0x31F0 && sym <= 0x31FF) /* Katakana Phonetic Extensions */
		return BOOL_TRUE;
	if (sym >= 0x3200 && sym <= 0x32FF) /* Enclosed CJK Letters and Months */
		return BOOL_TRUE;
	if (sym >= 0x3300 && sym <= 0x33FF) /* CJK Compatibility */
		return BOOL_TRUE;
	if (sym >= 0x3400 && sym <= 0x4DBF) /* CJK Unified Ideographs Extension A */
		return BOOL_TRUE;
	if (sym >= 0x4DC0 && sym <= 0x4DFF) /* Yijing Hexagram Symbols */
		return BOOL_TRUE;
	if (sym >= 0x4E00 && sym <= 0x9FFF) /* CJK Unified Ideographs */
		return BOOL_TRUE;
	if (sym >= 0xA000 && sym <= 0xA48F) /* Yi Syllables */
		return BOOL_TRUE;
	if (sym >= 0xA490 && sym <= 0xA4CF) /* Yi Radicals */
		return BOOL_TRUE;
#endif
	/* Speed up previous block */
	if (sym >= 0x2E80 && sym <= 0xA4CF && sym != 0x303F)
		return BOOL_TRUE;

	if (sym >= 0xAC00 && sym <= 0xD7AF) /* Hangul Syllables */
		return BOOL_TRUE;
	if (sym >= 0xF900 && sym <= 0xFAFF) /* CJK Compatibility Ideographs */
		return BOOL_TRUE;
	if (sym >= 0xFE10 && sym <= 0xFE1F) /* Vertical Forms */
		return BOOL_TRUE;

#if 0
	if (sym >= 0xFE30 && sym <= 0xFE4F) /* CJK Compatibility Forms */
		return BOOL_TRUE;
	if (sym >= 0xFE50 && sym <= 0xFE6F) /* Small Form Variants */
		return BOOL_TRUE;
#endif
	/* Speed up previous block */
	if (sym >= 0xFE30 && sym <= 0xFE6F)
		return BOOL_TRUE;

	if ((sym >= 0xFF00 && sym <= 0xFF60) ||
		(sym >= 0xFFE0 && sym <= 0xFFE6)) /* Fullwidth Forms */
		return BOOL_TRUE;

	if (sym >= 0x1D300 && sym <= 0x1D35F) /* Tai Xuan Jing Symbols */
		return BOOL_TRUE;
	if (sym >= 0x20000 && sym <= 0x2B81F) /* CJK Unified Ideographs Extensions B, C, D */
		return BOOL_TRUE;
	if (sym >= 0x2F800 && sym <= 0x2FA1F) /* CJK Compatibility Ideographs Supplement */
		return BOOL_TRUE;

	return BOOL_FALSE;
}


/** @brief Get position of previous UTF-8 char within string
 *
 * @param [in] line UTF-8 string.
 * @param [in] cur_pos Current position within UTF-8 string.
 * @return Position of previous UTF-8 character or NULL on error.
 */
size_t utf8_move_left(const char *line, size_t cur_pos)
{
	const char *pos = line + cur_pos;

	if (!line)
		return 0;
	if (pos == line)
		return cur_pos; // It's already leftmost position
	do {
		pos--;
	} while ((pos > line) && (UTF8_10 == (*pos & UTF8_MASK)));

	return pos - line;
}


/** @brief Get position of next UTF-8 char within string
 *
 * @param [in] line UTF-8 string.
 * @param [in] cur_pos Current position within UTF-8 string.
 * @return Position of next UTF-8 character or NULL on error.
 */
size_t utf8_move_right(const char *line, size_t cur_pos)
{
	const char *pos = line + cur_pos;

	if (!line)
		return 0;
	if (*pos == '\0')
		return cur_pos; // It's already rightmost position
	do {
		pos++;
	} while ((*pos != '\0') && (UTF8_10 == (*pos & UTF8_MASK)));

	return pos - line;
}


/** @brief Counts number of printable symbols within UTF-8 string
 *
 * One printable symbol can consist of several UTF-8 bytes.
 * CJK UTF-8 character can occupy 2 printable positions.
 *
 * @param [in] str UTF-8 string.
 * @param [in] end End of line position (pointer). Can be NULL - no limit.
 * @param Number of printable symbols or < 0 on error.
 */
ssize_t utf8_nsyms(const char *str, size_t len)
{
	const char *pos = str;
	ssize_t nsym = 0;

	if (!str)
		return -1;

	while ((pos < (str + len)) && (*pos != '\0')) {
		unsigned long sym = 0;

		// ASCII char
		if ((UTF8_7BIT_MASK & *pos) == 0) {
			pos++;
			nsym++;
			continue;
		}

		// Multibyte UTF-8
		pos += utf8_to_wchar(pos, &sym);
		if (utf8_wchar_is_cjk(sym)) // CJK chars have double-width
			nsym += 2;
		else
			nsym += 1;
	}

	return nsym;
}
