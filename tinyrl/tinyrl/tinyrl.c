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

#include <faux/faux.h>
#include <faux/str.h>

#include "private.h"

#define LINE_CHUNK 80


tinyrl_t *tinyrl_new(FILE *istream, FILE *ostream,
	const char *hist_fname, size_t hist_stifle)
{
	tinyrl_t *tinyrl = NULL;
	int i = 0;

	tinyrl = faux_zmalloc(sizeof(tinyrl_t));
	if (!tinyrl)
		return NULL;

	// Line
	faux_bzero(&tinyrl->line, sizeof(tinyrl->line));
	tinyrl_line_extend(tinyrl, LINE_CHUNK);

	// Last line
	tinyrl_reset_line_state(tinyrl);

	// Input processing vars
	tinyrl->utf8_cont = 0;
	tinyrl->esc_cont = BOOL_FALSE;
	tinyrl->esc_seq[0] = '\0';
	tinyrl->esc_p = tinyrl->esc_seq;

	// Prompt
	tinyrl_set_prompt(tinyrl, "> ");

	// Key handlers
	for (i = 0; i < NUM_HANDLERS; i++) {
		tinyrl->handlers[i] = tinyrl_key_default;
	}
	tinyrl->handlers[KEY_CR] = tinyrl_key_crlf;
	tinyrl->handlers[KEY_LF] = tinyrl_key_crlf;
	tinyrl->handlers[KEY_ETX] = tinyrl_key_interrupt;
	tinyrl->handlers[KEY_DEL] = tinyrl_key_backspace;
	tinyrl->handlers[KEY_BS] = tinyrl_key_backspace;
	tinyrl->handlers[KEY_EOT] = tinyrl_key_delete;
	tinyrl->handlers[KEY_FF] = tinyrl_key_clear_screen;
	tinyrl->handlers[KEY_NAK] = tinyrl_key_erase_line;
	tinyrl->handlers[KEY_SOH] = tinyrl_key_start_of_line;
	tinyrl->handlers[KEY_ENQ] = tinyrl_key_end_of_line;
	tinyrl->handlers[KEY_VT] = tinyrl_key_kill;
	tinyrl->handlers[KEY_EM] = tinyrl_key_yank;
	tinyrl->handlers[KEY_HT] = tinyrl_key_tab;
	tinyrl->handlers[KEY_ETB] = tinyrl_key_backword;

	tinyrl->max_line_length = 0;
	tinyrl->buffer = NULL;
	tinyrl->buffer_size = 0;
	tinyrl->done = BOOL_FALSE;
	tinyrl->completion_over = BOOL_FALSE;
	tinyrl->attempted_completion_function = NULL;
	tinyrl->hotkey_fn = NULL;
	tinyrl->state = 0;
	tinyrl->kill_string = NULL;
	tinyrl->echo_char = '\0';
	tinyrl->echo_enabled = BOOL_TRUE;
	tinyrl->utf8 = BOOL_TRUE;
	tinyrl->busy = BOOL_FALSE;

	// VT100 terminal
	tinyrl->term = vt100_new(istream, ostream);
	// To save terminal settings
	tinyrl_set_istream(tinyrl, istream);
	tinyrl->width = vt100_width(tinyrl->term);

	// History object
	tinyrl->hist = hist_new(hist_fname, hist_stifle);
	tinyrl_hist_restore(tinyrl);

	tty_raw_mode(tinyrl);

	return tinyrl;
}


void tinyrl_free(tinyrl_t *tinyrl)
{
	assert(tinyrl);
	if (!tinyrl)
		return;

	tty_restore_mode(tinyrl);

	tinyrl_hist_save(tinyrl);
	hist_free(tinyrl->hist);

	vt100_free(tinyrl->term);
	faux_str_free(tinyrl->prompt);
	tinyrl_reset_line_state(tinyrl); // It's really reset 'last' string
	faux_str_free(tinyrl->line.str);

	faux_free(tinyrl);
}


void tty_raw_mode(tinyrl_t *tinyrl)
{
	struct termios new_termios = {};
	FILE *istream = NULL;
	int fd = -1;

	if (!tinyrl)
		return;
	istream = vt100_istream(tinyrl->term);
	if (!istream)
		return;
	fd = fileno(istream);
	if (tcgetattr(fd, &new_termios) < 0)
		return;
	new_termios.c_iflag = 0;
	new_termios.c_oflag = OPOST | ONLCR;
	new_termios.c_lflag = 0;
	new_termios.c_cc[VMIN] = 1;
	new_termios.c_cc[VTIME] = 0;
	// Mode switch
	tcsetattr(fd, TCSADRAIN, &new_termios);
}


void tty_restore_mode(tinyrl_t *tinyrl)
{
	FILE *istream = NULL;
	int fd = -1;

	istream = vt100_istream(tinyrl->term);
	if (!istream)
		return;
	fd = fileno(istream);
	// Do the mode switch
	tcsetattr(fd, TCSADRAIN, &tinyrl->default_termios);
}


bool_t tinyrl_bind_key(tinyrl_t *tinyrl, int key, tinyrl_key_func_t *fn)
{
	assert(tinyrl);
	if (!tinyrl)
		return BOOL_FALSE;
	if ((key < 0) || (key > 255))
		return BOOL_FALSE;

	tinyrl->handlers[key] = fn;

	return BOOL_TRUE;
}


void tinyrl_set_hotkey_fn(tinyrl_t *tinyrl, tinyrl_key_func_t *fn)
{
	tinyrl->hotkey_fn = fn;
}


void tinyrl_set_istream(tinyrl_t *tinyrl, FILE *istream)
{
	assert(tinyrl);
	if (!tinyrl)
		return;

	vt100_set_istream(tinyrl->term, istream);
	// Save terminal settings to restore on exit
	if (istream)
		tcgetattr(fileno(istream), &tinyrl->default_termios);
}


FILE *tinyrl_istream(const tinyrl_t *tinyrl)
{
	return vt100_istream(tinyrl->term);
}


void tinyrl_set_ostream(tinyrl_t *tinyrl, FILE *ostream)
{
	assert(tinyrl);
	if (!tinyrl)
		return;

	vt100_set_ostream(tinyrl->term, ostream);
}


FILE *tinyrl_ostream(const tinyrl_t *tinyrl)
{
	return vt100_ostream(tinyrl->term);
}


bool_t tinyrl_utf8(const tinyrl_t *tinyrl)
{
	assert(tinyrl);
	if (!tinyrl)
		return BOOL_TRUE;

	return tinyrl->utf8;
}


void tinyrl_set_utf8(tinyrl_t *tinyrl, bool_t utf8)
{
	assert(tinyrl);
	if (!tinyrl)
		return;

	tinyrl->utf8 = utf8;
}


bool_t tinyrl_busy(const tinyrl_t *tinyrl)
{
	assert(tinyrl);
	if (!tinyrl)
		return BOOL_FALSE;

	return tinyrl->busy;
}


void tinyrl_set_busy(tinyrl_t *tinyrl, bool_t busy)
{
	assert(tinyrl);
	if (!tinyrl)
		return;

	tinyrl->busy = busy;
}


const char *tinyrl_prompt(const tinyrl_t *tinyrl)
{
	assert(tinyrl);
	if (!tinyrl)
		return NULL;

	return tinyrl->prompt;
}


void tinyrl_set_prompt(tinyrl_t *tinyrl, const char *prompt)
{
	const char *last_cr = NULL;

	assert(tinyrl);
	if (!tinyrl)
		return;

	if (tinyrl->prompt)
		faux_str_free(tinyrl->prompt);
	tinyrl->prompt = faux_str_dup(prompt);
	tinyrl->prompt_len = strlen(tinyrl->prompt);

	// Prompt can contain '\n' characters so let prompt_chars count symbols
	// of last line only.
	last_cr = strrchr(tinyrl->prompt, '\n');
	if (!last_cr)
		last_cr = tinyrl->prompt;
	else
		last_cr++; // Skip '\n' itself
	tinyrl->prompt_chars = utf8_nsyms(last_cr, tinyrl->prompt_len);
}


void *tinyrl_udata(const tinyrl_t *tinyrl)
{
	assert(tinyrl);
	if (!tinyrl)
		return NULL;

	return tinyrl->udata;
}


void tinyrl_set_udata(tinyrl_t *tinyrl, void *udata)
{
	assert(tinyrl);
	if (!tinyrl)
		return;

	tinyrl->udata = udata;
}


const char *tinyrl_line(const tinyrl_t *tinyrl)
{
	return tinyrl->line.str;
}


bool_t tinyrl_hist_save(const tinyrl_t *tinyrl)
{
	assert(tinyrl);
	if (!tinyrl)
		return BOOL_FALSE;

	return hist_save(tinyrl->hist);
}


bool_t tinyrl_hist_restore(tinyrl_t *tinyrl)
{
	assert(tinyrl);
	if (!tinyrl)
		return BOOL_FALSE;

	return hist_restore(tinyrl->hist);
}


static bool_t process_char(tinyrl_t *tinyrl, char key)
{

	// Begin of ESC sequence
	if (!tinyrl->esc_cont && (KEY_ESC == key)) {
		tinyrl->esc_cont = BOOL_TRUE; // Start ESC sequence
		tinyrl->esc_p = tinyrl->esc_seq;
		// Note: Don't put ESC symbol itself to buffer
		return BOOL_TRUE;
	}

	// Continue ESC sequence
	if (tinyrl->esc_cont) {
		// Broken sequence. Too long
		if ((tinyrl->esc_p - tinyrl->esc_seq) >= (sizeof(tinyrl->esc_seq) - 1)) {
			tinyrl->esc_cont = BOOL_FALSE;
			return BOOL_FALSE;
		}
		// Save the curren char to sequence buffer
		*tinyrl->esc_p = key;
		tinyrl->esc_p++;
		// ANSI standard control sequences will end
		// with a character between 64 - 126
		if ((key != '[') && (key > 63)) {
			*tinyrl->esc_p = '\0';
			tinyrl_esc_seq(tinyrl, tinyrl->esc_seq);
			tinyrl->esc_cont = BOOL_FALSE;
			//tinyrl_redisplay(tinyrl);
		}
		return BOOL_TRUE;
	}

	// Call the handler for key
	// Handler (that has no special meaning) will put new char to line buffer
	if (!tinyrl->handlers[(unsigned char)key](tinyrl, key))
		vt100_ding(tinyrl->term);

//	if (tinyrl->done) // Some handler set the done flag
//		continue; // It will break the loop

	if (tinyrl->utf8) {
		 // ASCII char (one byte)
		if (!(UTF8_7BIT_MASK & key)) {
			tinyrl->utf8_cont = 0;
		// First byte of multibyte symbol
		} else if (UTF8_11 == (key & UTF8_MASK)) {
			// Find out number of symbol's bytes
			unsigned int b = (unsigned int)key;
			tinyrl->utf8_cont = 0;
			while ((tinyrl->utf8_cont < 6) && (UTF8_10 != (b & UTF8_MASK))) {
				tinyrl->utf8_cont++;
				b = b << 1;
			}
		// Continue of multibyte symbol
		} else if ((tinyrl->utf8_cont > 0) && (UTF8_10 == (key & UTF8_MASK))) {
			tinyrl->utf8_cont--;
		}
	}

	// For non UTF-8 encoding the utf8_cont is always 0.
	// For UTF-8 it's 0 when one-byte symbol or we get
	// all bytes for the current multibyte character
//	if (!tinyrl->utf8_cont) {
//		//tinyrl_redisplay(tinyrl);
//		printf("%s\n", tinyrl->line.str);
//	}

	return BOOL_TRUE;
}


int tinyrl_read(tinyrl_t *tinyrl)
{
	int rc = 0;
	unsigned char key = 0;
	int count = 0;

	assert(tinyrl);

	tinyrl_set_busy(tinyrl, BOOL_FALSE);

	while ((rc = vt100_getchar(tinyrl->term, &key)) > 0) {
		count++;
		process_char(tinyrl, key);
		// Some commands can't be processed immediately by handlers and
		// need some network exchange for example. In this case we will
		// not execute redisplay() here.
		if (!tinyrl->utf8_cont && !tinyrl_busy(tinyrl)) {
			tinyrl_redisplay(tinyrl);
	//		printf("%s\n", tinyrl->line.str);
		}
//printf("key=%u, pos=%lu, len=%lu\n", key, tinyrl->line.pos, tinyrl->line.len);
	}

	if ((rc < 0) && (EAGAIN == errno))
		return count;

	return rc;
}


/*
 * Ensure that buffer has enough space to hold len characters,
 * possibly reallocating it if necessary. The function returns BOOL_TRUE
 * if the line is successfully extended, BOOL_FALSE if not.
 */
bool_t tinyrl_line_extend(tinyrl_t *tinyrl, size_t len)
{
	char *new_buf = NULL;
	size_t new_size = 0;
	size_t chunk_num = 0;

	if (tinyrl->line.len >= len)
		return BOOL_TRUE;

	chunk_num = len / LINE_CHUNK;
	if ((len % LINE_CHUNK) > 0)
		chunk_num++;
	new_size = chunk_num * LINE_CHUNK;

	// First initialization
	if (tinyrl->line.str == NULL) {
		tinyrl->line.str = faux_zmalloc(new_size);
		if (!tinyrl->line.str)
			return BOOL_FALSE;
		tinyrl->line.size = new_size;
		return BOOL_TRUE;
	}

	new_buf = realloc(tinyrl->line.str, new_size);
	if (!new_buf)
		return BOOL_FALSE;
	tinyrl->line.str = new_buf;
	tinyrl->line.size = new_size;

	return BOOL_TRUE;
}


bool_t tinyrl_esc_seq(tinyrl_t *tinyrl, const char *esc_seq)
{
	bool_t result = BOOL_FALSE;

	switch (vt100_esc_decode(tinyrl->term, esc_seq)) {
	case VT100_CURSOR_UP:
		result = tinyrl_key_up(tinyrl, 0);
		break;
	case VT100_CURSOR_DOWN:
		result = tinyrl_key_down(tinyrl, 0);
		break;
	case VT100_CURSOR_LEFT:
		result = tinyrl_key_left(tinyrl, 0);
		break;
	case VT100_CURSOR_RIGHT:
		result = tinyrl_key_right(tinyrl, 0);
		break;
	case VT100_HOME:
		result = tinyrl_key_start_of_line(tinyrl, 0);
		break;
	case VT100_END:
		result = tinyrl_key_end_of_line(tinyrl, 0);
		break;
	case VT100_DELETE:
		result = tinyrl_key_delete(tinyrl, 0);
		break;
	case VT100_INSERT:
	case VT100_PGDOWN:
	case VT100_PGUP:
	case VT100_UNKNOWN:
		break;
	}

	return result;
}


bool_t tinyrl_line_insert(tinyrl_t *tinyrl, const char *text, size_t len)
{
	size_t new_size = tinyrl->line.len + len + 1;

	if (len == 0)
		return BOOL_TRUE;

	tinyrl_line_extend(tinyrl, new_size);

	if (tinyrl->line.pos < tinyrl->line.len) {
		memmove(tinyrl->line.str + tinyrl->line.pos + len,
			tinyrl->line.str + tinyrl->line.pos,
			tinyrl->line.len - tinyrl->line.pos);
	}

	memcpy(tinyrl->line.str + tinyrl->line.pos, text, len);
	tinyrl->line.pos += len;
	tinyrl->line.len += len;
	tinyrl->line.str[tinyrl->line.len] = '\0';

	return BOOL_TRUE;
}


bool_t tinyrl_line_delete(tinyrl_t *tinyrl, off_t start, size_t len)
{
	if (start >= tinyrl->line.len)
		return BOOL_TRUE;

	if ((start + len) >= tinyrl->line.len) {
		tinyrl->line.len = start;
	} else {
		memmove(tinyrl->line.str + start,
			tinyrl->line.str + start + len,
			tinyrl->line.len - (start + len));
		tinyrl->line.len -= len;
	}

	tinyrl->line.pos = start;
	tinyrl->line.str[tinyrl->line.len] = '\0';

	return BOOL_TRUE;
}


bool_t tinyrl_line_replace(tinyrl_t *tinyrl, const char *text)
{
	size_t len = 0;

	if (faux_str_is_empty(text)) {
		tinyrl_reset_line(tinyrl);
		return BOOL_TRUE;
	}

	len = strlen(text);
	tinyrl_line_extend(tinyrl, len + 1);

	memcpy(tinyrl->line.str, text, len);
	tinyrl->line.pos = len;
	tinyrl->line.len = len;
	tinyrl->line.str[tinyrl->line.len] = '\0';

	return BOOL_TRUE;
}


static void move_cursor(const tinyrl_t *tinyrl, size_t cur_pos, size_t target_pos)
{
	int rows = 0;
	int cols = 0;

	// Note: The '/' is not real math division. It's integer part of division
	// so we need separate division for two values.
	rows = (target_pos / tinyrl->width) - (cur_pos / tinyrl->width);
	cols = (target_pos % tinyrl->width) - (cur_pos % tinyrl->width);

	if (cols > 0)
		vt100_cursor_forward(tinyrl->term, cols);
	else if (cols < 0)
		vt100_cursor_back(tinyrl->term, -cols);

	if (rows > 0)
		vt100_cursor_down(tinyrl->term, rows);
	else if (rows < 0)
		vt100_cursor_up(tinyrl->term, -rows);
}


size_t tinyrl_equal_part(const tinyrl_t *tinyrl,
	const char *s1, const char *s2)
{
	const char *str1 = s1;
	const char *str2 = s2;

	if (!str1 || !str2)
		return 0;

	while (*str1 && *str2) {
		if (*str1 != *str2)
			break;
		str1++;
		str2++;
	}
	if (!tinyrl->utf8)
		return str1 - s1;

	// UTF8
	// If UTF8_10 byte (intermediate byte of UTF-8 sequence) is not equal
	// then we need to find starting of this UTF-8 character because whole
	// UTF-8 character is not equal.
	if (UTF8_10 == (*str1 & UTF8_MASK)) {
		// Skip intermediate bytes
		while ((str1 > s1) && (UTF8_10 == (*str1 & UTF8_MASK)))
			str1--;
	}

	return str1 - s1;
}


void tinyrl_save_last(tinyrl_t *tinyrl)
{
	faux_str_free(tinyrl->last.str);
	tinyrl->last = tinyrl->line;
	tinyrl->last.str = faux_str_dup(tinyrl->line.str);
}


void tinyrl_reset_line_state(tinyrl_t *tinyrl)
{
	faux_str_free(tinyrl->last.str);
	faux_bzero(&tinyrl->last, sizeof(tinyrl->last));
}


void tinyrl_reset_line(tinyrl_t *tinyrl)
{
	tinyrl_line_delete(tinyrl, 0, tinyrl->line.len);
}


void tinyrl_redisplay(tinyrl_t *tinyrl)
{
	size_t width = vt100_width(tinyrl->term);
//	unsigned int line_size = strlen(tinyrl->line);
	unsigned int line_chars = utf8_nsyms(tinyrl->line.str, tinyrl->line.len);
	size_t cols = 0;
	size_t eq_bytes = 0;

	// Prepare print position
	if (tinyrl->last.str && (width == tinyrl->width)) {
		size_t eq_chars = 0; // Printable symbols
		size_t last_pos_chars = 0;
		// If line and last line have the equal chars at begining
		eq_bytes = tinyrl_equal_part(tinyrl, tinyrl->line.str, tinyrl->last.str);
		eq_chars = utf8_nsyms(tinyrl->last.str, eq_bytes);
		last_pos_chars = utf8_nsyms(tinyrl->last.str, tinyrl->last.pos);
		move_cursor(tinyrl, tinyrl->prompt_chars + last_pos_chars,
			tinyrl->prompt_chars + eq_chars);
	} else {
		// Prepare to resize
		if (width != tinyrl->width) {
			vt100_next_line(tinyrl->term);
			vt100_erase_down(tinyrl->term);
		}
		vt100_printf(tinyrl->term, "%s", tinyrl->prompt);
	}

	// Print current line
	vt100_printf(tinyrl->term, "%s", tinyrl->line.str + eq_bytes);
	cols = (tinyrl->prompt_chars + line_chars) % width;
	if ((cols == 0) && (tinyrl->line.len - eq_bytes))
		vt100_next_line(tinyrl->term);
	// Erase down if current line is shorter than previous one
	if (tinyrl->last.len > tinyrl->line.len)
		vt100_erase_down(tinyrl->term);
	// Move the cursor to the insertion point
	if (tinyrl->line.pos < tinyrl->line.len) {
		size_t pos_chars = utf8_nsyms(tinyrl->line.str, tinyrl->line.pos);
		move_cursor(tinyrl, tinyrl->prompt_chars + line_chars,
			tinyrl->prompt_chars + pos_chars);
	}

	// Update the display
	vt100_oflush(tinyrl->term);

	// Save the last line buffer
	tinyrl_save_last(tinyrl);

	tinyrl->width = width;
}


void tinyrl_crlf(const tinyrl_t *tinyrl)
{
	vt100_printf(tinyrl->term, "\n");
}


// Jump to first free line after current multiline input
void tinyrl_multi_crlf(const tinyrl_t *tinyrl)
{
	size_t full_chars = utf8_nsyms(tinyrl->last.str, tinyrl->last.len);
	size_t pos_chars = utf8_nsyms(tinyrl->last.str, tinyrl->last.pos);

	move_cursor(tinyrl, tinyrl->prompt_chars + pos_chars,
		tinyrl->prompt_chars + full_chars);
	tinyrl_crlf(tinyrl);
	vt100_oflush(tinyrl->term);
}


void tinyrl_line_to_hist(tinyrl_t *tinyrl)
{
	if (tinyrl->line.len == 0)
		return;

	hist_add(tinyrl->hist, tinyrl->line.str, BOOL_FALSE);
}


void tinyrl_reset_hist_pos(tinyrl_t *tinyrl)
{
	hist_pos_reset(tinyrl->hist);
}


size_t tinyrl_width(const tinyrl_t *tinyrl)
{
	assert(tinyrl);
	if (!tinyrl)
		return 80;

	return tinyrl->width;
}


// Because terminal is in raw mode and standard libc printf() can don't flush()
int tinyrl_printf(const tinyrl_t *tinyrl, const char *fmt, ...)
{
	va_list args;
	int len = 0;

	va_start(args, fmt);
	len = vt100_vprintf(tinyrl->term, fmt, args);
	va_end(args);

	return len;
}
