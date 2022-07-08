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

	// Input processing vars
	tinyrl->utf8_cont = 0;
	tinyrl->esc_cont = BOOL_FALSE;
	tinyrl->esc_seq[0] = '\0';
	tinyrl->esc_p = tinyrl->esc_seq;

	// Prompt
	tinyrl->prompt = faux_str_dup("> ");
	tinyrl->prompt_len = strlen(tinyrl->prompt);
	tinyrl->prompt_chars = utf8_nsyms(tinyrl->prompt, tinyrl->prompt_len);

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
	tinyrl->last_buffer = NULL;
	tinyrl->last_point = 0;
	tinyrl->last_line_size = 0;
	tinyrl->utf8 = BOOL_TRUE;

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

	hist_free(tinyrl->hist);
	vt100_free(tinyrl->term);

	faux_str_free(tinyrl->prompt);

	faux_str_free(tinyrl->buffer);
	faux_str_free(tinyrl->kill_string);
	faux_str_free(tinyrl->last_buffer);

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

	while ((rc = vt100_getchar(tinyrl->term, &key)) > 0) {
		count++;
		process_char(tinyrl, key);
		if (!tinyrl->utf8_cont) {
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


static size_t str_equal_part(const tinyrl_t *tinyrl,
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
		eq_bytes = str_equal_part(tinyrl, tinyrl->line.str, tinyrl->last.str);
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
//		count = utf8_nsyms(tinyrl, tinyrl->line + tinyrl->point,
//			line_size - tinyrl->point);
		move_cursor(tinyrl, tinyrl->prompt_chars + line_chars,
			tinyrl->prompt_chars + pos_chars);
	}

	// Update the display
	vt100_oflush(tinyrl->term);

	// Save the last line buffer
	faux_str_free(tinyrl->last.str);
	tinyrl->last = tinyrl->line;
	tinyrl->last.str = faux_str_dup(tinyrl->line.str);

	tinyrl->width = width;
}


#if 0

/*----------------------------------------------------------------------- */
/*
tinyrl is called whenever a line is edited in any way.
It signals that if we are currently viewing a history line we should transfer it
to the current buffer
*/
static void changed_line(tinyrl_t * tinyrl)
{
	/* if the current line is not our buffer then make it so */
	if (tinyrl->line != tinyrl->buffer) {
		/* replace the current buffer with the new details */
		free(tinyrl->buffer);
		tinyrl->line = tinyrl->buffer = lub_string_dup(tinyrl->line);
		tinyrl->buffer_size = strlen(tinyrl->buffer);
		assert(tinyrl->line);
	}
}



/*-------------------------------------------------------- */
/* Jump to first free line after current multiline input   */
void tinyrl_multi_crlf(const tinyrl_t * tinyrl)
{
	unsigned int line_size = strlen(tinyrl->last_buffer);
	unsigned int line_len = utf8_nsyms(tinyrl, tinyrl->last_buffer, line_size);
	unsigned int count = utf8_nsyms(tinyrl, tinyrl->last_buffer, tinyrl->last_point);

	tinyrl_internal_position(tinyrl, tinyrl->prompt_len + line_len,
		- (line_len - count), tinyrl->width);
	tinyrl_crlf(tinyrl);
	tinyrl_vt100_oflush(tinyrl->term);
}



/*----------------------------------------------------------------------- */
/* 
 * A convenience function for displaying a list of strings in columnar
 * format on Readline's output stream. matches is the list of strings,
 * in argv format, such as a list of completion matches. len is the number
 * of strings in matches, and max is the length of the longest string in matches.
 * tinyrl function uses the setting of print-completions-horizontally to select
 * how the matches are displayed
 */
void tinyrl_display_matches(const tinyrl_t *tinyrl,
	char *const *matches, unsigned int len, size_t max)
{
	unsigned int width = tinyrl_vt100__get_width(tinyrl->term);
	unsigned int cols, rows;

	/* Find out column and rows number */
	if (max < width)
		cols = (width + 1) / (max + 1); /* allow for a space between words */
	else
		cols = 1;
	rows = len / cols + 1;

	assert(matches);
	if (matches) {
		unsigned int r, c;
		len--, matches++; /* skip the subtitution string */
		/* Print out a table of completions */
		for (r = 0; r < rows && len; r++) {
			for (c = 0; c < cols && len; c++) {
				const char *match = *matches++;
				len--;
				if ((c + 1) == cols) /* Last str in row */
					tinyrl_vt100_printf(tinyrl->term, "%s",
						match);
				else
					tinyrl_vt100_printf(tinyrl->term, "%-*s ",
						max, match);
			}
			tinyrl_crlf(tinyrl);
		}
	}
}


/*-------------------------------------------------------- */
/*
 * Returns an array of strings which is a list of completions for text.
 * If there are no completions, returns NULL. The first entry in the
 * returned array is the substitution for text. The remaining entries
 * are the possible completions. The array is terminated with a NULL pointer.
 *
 * entry_func is a function of two args, and returns a char *. 
 * The first argument is text. The second is a state argument;
 * it is zero on the first call, and non-zero on subsequent calls.
 * entry_func returns a NULL pointer to the caller when there are no 
 * more matches. 
 */
char **tinyrl_completion(tinyrl_t * tinyrl,
	const char *line, unsigned int start, unsigned int end,
	tinyrl_compentry_func_t * entry_func)
{
	unsigned int state = 0;
	size_t size = 1;
	unsigned int offset = 1; /* Need at least one entry for the substitution */
	char **matches = NULL;
	char *match;
	/* duplicate the string upto the insertion point */
	char *text = lub_string_dupn(line, end);

	/* now try and find possible completions */
	while ((match = entry_func(tinyrl, text, start, state++))) {
		if (size == offset) {
			/* resize the buffer if needed - the +1 is for the NULL terminator */
			size += 10;
			matches =
			    realloc(matches, (sizeof(char *) * (size + 1)));
		}
		/* not much we can do... */
		if (!matches)
			break;
		matches[offset] = match;

		/*
		 * augment the substitute string with tinyrl entry
		 */
		if (1 == offset) {
			/* let's be optimistic */
			matches[0] = lub_string_dup(match);
		} else {
			char *p = matches[0];
			size_t match_len = strlen(p);
			/* identify the common prefix */
			while ((tolower(*p) == tolower(*match)) && match_len--) {
				p++, match++;
			}
			/* terminate the prefix string */
			*p = '\0';
		}
		offset++;
	}
	/* be a good memory citizen */
	lub_string_free(text);

	if (matches)
		matches[offset] = NULL;
	return matches;
}

/*-------------------------------------------------------- */
void tinyrl_delete_matches(char **tinyrl)
{
	char **matches = tinyrl;
	while (*matches) {
		/* release the memory for each contained string */
		free(*matches++);
	}
	/* release the memory for the array */
	free(tinyrl);
}

/*-------------------------------------------------------- */
void tinyrl_crlf(const tinyrl_t * tinyrl)
{
	tinyrl_vt100_printf(tinyrl->term, "\n");
}

/*-------------------------------------------------------- */
/*
 * Ring the terminal bell, obeying the setting of bell-style.
 */
void tinyrl_ding(const tinyrl_t * tinyrl)
{
	tinyrl_vt100_ding(tinyrl->term);
}

/*-------------------------------------------------------- */
void tinyrl_reset_line_state(tinyrl_t * tinyrl)
{
	lub_string_free(tinyrl->last_buffer);
	tinyrl->last_buffer = NULL;
	tinyrl->last_line_size = 0;

	tinyrl_redisplay(tinyrl);
}

/*-------------------------------------------------------- */
void tinyrl_replace_line(tinyrl_t * tinyrl, const char *text, int clear_undo)
{
	size_t new_len = strlen(text);

	/* ignored for now */
	clear_undo = clear_undo;

	/* ensure there is sufficient space */
	if (tinyrl_extend_line_buffer(tinyrl, new_len)) {

		/* overwrite the current contents of the buffer */
		strcpy(tinyrl->buffer, text);

		/* set the insert point and end point */
		tinyrl->point = tinyrl->end = new_len;
	}
	tinyrl_redisplay(tinyrl);
}

/*-------------------------------------------------------- */
static tinyrl_match_e
tinyrl_do_complete(tinyrl_t * tinyrl, bool_t with_extensions)
{
	tinyrl_match_e result = TINYRL_NO_MATCH;
	char **matches = NULL;
	unsigned int start, end;
	bool_t completion = BOOL_FALSE;
	bool_t prefix = BOOL_FALSE;
	int i = 0;

	/* find the start and end of the current word */
	start = end = tinyrl->point;
	while (start && !isspace(tinyrl->line[start - 1]))
		start--;

	if (tinyrl->attempted_completion_function) {
		tinyrl->completion_over = BOOL_FALSE;
		tinyrl->completion_error_over = BOOL_FALSE;
		/* try and complete the current line buffer */
		matches = tinyrl->attempted_completion_function(tinyrl,
			tinyrl->line, start, end);
	}
	if (!matches && (BOOL_FALSE == tinyrl->completion_over)) {
		/* insert default completion call here... */
	}
	if (!matches)
		return result;

	/* identify and insert a common prefix if there is one */
	if (0 != strncmp(matches[0], &tinyrl->line[start],
		strlen(matches[0]))) {
		/*
		 * delete the original text not including
		 * the current insertion point character
		 */
		if (tinyrl->end != end)
			end--;
		tinyrl_delete_text(tinyrl, start, end);
		if (BOOL_FALSE == tinyrl_insert_text(tinyrl, matches[0]))
			return TINYRL_NO_MATCH;
		completion = BOOL_TRUE;
	}
	for (i = 1; matches[i]; i++) {
		/* tinyrl is just a prefix string */
		if (0 == lub_string_nocasecmp(matches[0], matches[i]))
			prefix = BOOL_TRUE;
	}
	/* is there more than one completion? */
	if (matches[2]) {
		char **tmp = matches;
		unsigned int max, len;
		max = len = 0;
		while (*tmp) {
			size_t size = strlen(*tmp++);
			len++;
			if (size > max)
				max = size;
		}
		if (completion)
			result = TINYRL_COMPLETED_AMBIGUOUS;
		else if (prefix)
			result = TINYRL_MATCH_WITH_EXTENSIONS;
		else
			result = TINYRL_AMBIGUOUS;
		if (with_extensions || !prefix) {
			/* Either we always want to show extensions or
			 * we haven't been able to complete the current line
			 * and there is just a prefix, so let the user see the options
			 */
			tinyrl_crlf(tinyrl);
			tinyrl_display_matches(tinyrl, matches, len, max);
			tinyrl_reset_line_state(tinyrl);
		}
	} else {
		result = completion ?
			TINYRL_COMPLETED_MATCH : TINYRL_MATCH;
	}
	/* free the memory */
	tinyrl_delete_matches(matches);
	/* redisplay the line */
	tinyrl_redisplay(tinyrl);

	return result;
}

/*-------------------------------------------------------- */
tinyrl_match_e tinyrl_complete_with_extensions(tinyrl_t * tinyrl)
{
	return tinyrl_do_complete(tinyrl, BOOL_TRUE);
}

/*-------------------------------------------------------- */
tinyrl_match_e tinyrl_complete(tinyrl_t * tinyrl)
{
	return tinyrl_do_complete(tinyrl, BOOL_FALSE);
}

/*-------------------------------------------------------- */
void *tinyrl__get_context(const tinyrl_t * tinyrl)
{
	return tinyrl->context;
}

/*--------------------------------------------------------- */
const char *tinyrl__get_line(const tinyrl_t * tinyrl)
{
	return tinyrl->line;
}

/*--------------------------------------------------------- */
tinyrl_history_t *tinyrl__get_history(const tinyrl_t * tinyrl)
{
	return tinyrl->history;
}

/*--------------------------------------------------------- */
void tinyrl_completion_over(tinyrl_t * tinyrl)
{
	tinyrl->completion_over = BOOL_TRUE;
}

/*--------------------------------------------------------- */
void tinyrl_completion_error_over(tinyrl_t * tinyrl)
{
	tinyrl->completion_error_over = BOOL_TRUE;
}

/*--------------------------------------------------------- */
bool_t tinyrl_is_completion_error_over(const tinyrl_t * tinyrl)
{
	return tinyrl->completion_error_over;
}

/*--------------------------------------------------------- */
void tinyrl_done(tinyrl_t * tinyrl)
{
	tinyrl->done = BOOL_TRUE;
}

/*--------------------------------------------------------- */
void tinyrl_enable_echo(tinyrl_t * tinyrl)
{
	tinyrl->echo_enabled = BOOL_TRUE;
}

/*--------------------------------------------------------- */
void tinyrl_disable_echo(tinyrl_t * tinyrl, char echo_char)
{
	tinyrl->echo_enabled = BOOL_FALSE;
	tinyrl->echo_char = echo_char;
}


/*-------------------------------------------------------- */
const char *tinyrl__get_prompt(const tinyrl_t * tinyrl)
{
	return tinyrl->prompt;
}

/*-------------------------------------------------------- */
void tinyrl__set_prompt(tinyrl_t *tinyrl, const char *prompt)
{
	if (tinyrl->prompt) {
		lub_string_free(tinyrl->prompt);
		tinyrl->prompt_size = 0;
		tinyrl->prompt_len = 0;
	}
	tinyrl->prompt = lub_string_dup(prompt);
	if (tinyrl->prompt) {
		tinyrl->prompt_size = strlen(tinyrl->prompt);
		tinyrl->prompt_len = utf8_nsyms(tinyrl, tinyrl->prompt,
			tinyrl->prompt_size);
	}
}



/*-------------------------------------------------------- */
bool_t tinyrl_is_quoting(const tinyrl_t * tinyrl)
{
	bool_t result = BOOL_FALSE;
	/* count the quotes upto the current insertion point */
	unsigned int i = 0;
	while (i < tinyrl->point) {
		if (result && (tinyrl->line[i] == '\\')) {
			i++;
			if (i >= tinyrl->point)
				break;
			i++;
			continue;
		}
		if (tinyrl->line[i++] == '"') {
			result = result ? BOOL_FALSE : BOOL_TRUE;
		}
	}
	return result;
}

/*-------------------------------------------------------- */
bool_t tinyrl_is_empty(const tinyrl_t *tinyrl)
{
	return (tinyrl->point == 0) ? BOOL_TRUE : BOOL_FALSE;
}

/*--------------------------------------------------------- */
void tinyrl_limit_line_length(tinyrl_t * tinyrl, unsigned int length)
{
	tinyrl->max_line_length = length;
}


#endif
