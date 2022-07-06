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


tinyrl_t *tinyrl_new(FILE *istream, FILE *ostream,
	const char *hist_fname, size_t hist_stifle)
{
	tinyrl_t *tinyrl = NULL;
	int i = 0;

	tinyrl = faux_zmalloc(sizeof(tinyrl_t));
	if (!tinyrl)
		return NULL;

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

	tinyrl->line = NULL;
	tinyrl->max_line_length = 0;
	tinyrl->prompt = NULL;
	tinyrl->prompt_size = 0;
	tinyrl->buffer = NULL;
	tinyrl->buffer_size = 0;
	tinyrl->done = BOOL_FALSE;
	tinyrl->completion_over = BOOL_FALSE;
	tinyrl->point = 0;
	tinyrl->end = 0;
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

	faux_str_free(tinyrl->buffer);
	faux_str_free(tinyrl->kill_string);
	faux_str_free(tinyrl->last_buffer);
	faux_str_free(tinyrl->prompt);

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


static int process_char(tinyrl_t *tinyrl, unsigned char key)
{
#if 0
	FILE *istream = vt100_istream(tinyrl->term);
	char *result = NULL;
	int lerrno = 0;

	tinyrl->done = BOOL_FALSE;
	tinyrl->point = 0;
	tinyrl->end = 0;
	tinyrl->buffer = lub_string_dup("");
	tinyrl->buffer_size = strlen(tinyrl->buffer);
	tinyrl->line = tinyrl->buffer;
	tinyrl->context = context;

		unsigned int utf8_cont = 0; /* UTF-8 continue bytes */
		unsigned int esc_cont = 0; /* Escape sequence continues */
		char esc_seq[10]; /* Buffer for ESC sequence */
		char *esc_p = esc_seq;


			// Begin of ESC sequence
			if (!esc_cont && (KEY_ESC == key)) {
				esc_cont = 1; // Start ESC sequence
				esc_p = esc_seq;
				continue;
			}
			if (esc_cont) {
				/* Broken sequence */
				if (esc_p >= (esc_seq + sizeof(esc_seq) - 1)) {
					esc_cont = 0;
					continue;
				}
				/* Dump the control sequence into sequence buffer
				   ANSI standard control sequences will end
				   with a character between 64 - 126 */
				*esc_p = key & 0xff;
				esc_p++;
				/* tinyrl is an ANSI control sequence terminator code */
				if ((key != '[') && (key > 63)) {
					*esc_p = '\0';
					tinyrl_escape_seq(tinyrl, esc_seq);
					esc_cont = 0;
					tinyrl_redisplay(tinyrl);
				}
				continue;
			}

			/* Call the handler for tinyrl key */
			if (!tinyrl->handlers[key](tinyrl, key))
				tinyrl_ding(tinyrl);
			if (tinyrl->done) /* Some handler set the done flag */
				continue; /* It will break the loop */

			if (tinyrl->utf8) {
				if (!(UTF8_7BIT_MASK & key)) /* ASCII char */
					utf8_cont = 0;
				else if (utf8_cont && (UTF8_10 == (key & UTF8_MASK))) /* Continue byte */
					utf8_cont--;
				else if (UTF8_11 == (key & UTF8_MASK)) { /* First byte of multibyte char */
					/* Find out number of char's bytes */
					int b = key;
					utf8_cont = 0;
					while ((utf8_cont < 6) && (UTF8_10 != (b & UTF8_MASK))) {
						utf8_cont++;
						b = b << 1;
					}
				}
			}
			/* For non UTF-8 encoding the utf8_cont is always 0.
			   For UTF-8 it's 0 when one-byte symbol or we get
			   all bytes for the current multibyte character. */
			if (!utf8_cont)
				tinyrl_redisplay(tinyrl);
		}
		/* If the last character in the line (other than NULL)
		   is a space remove it. */
		if (tinyrl->end && tinyrl->line && isspace(tinyrl->line[tinyrl->end - 1]))
			tinyrl_delete_text(tinyrl, tinyrl->end - 1, tinyrl->end);
		/* Restores the terminal mode */
		tty_restore_mode(tinyrl);

	/*
	 * duplicate the string for return to the client 
	 * we have to duplicate as we may be referencing a
	 * history entry or our internal buffer
	 */
	result = tinyrl->line ? lub_string_dup(tinyrl->line) : NULL;

	/* free our internal buffer */
	free(tinyrl->buffer);
	tinyrl->buffer = NULL;

	if (!result)
		errno = lerrno; /* get saved errno */
	return result;
#endif
	return 1;
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
	}

	if ((rc < 0) && (EAGAIN == errno))
		return count;

	return rc;
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


static bool_t tinyrl_escape_seq(tinyrl_t *tinyrl, const char *esc_seq)
{
	int key = 0;
	bool_t result = BOOL_FALSE;

	switch (tinyrl_vt100_escape_decode(tinyrl->term, esc_seq)) {
	case tinyrl_vt100_CURSOR_UP:
		result = tinyrl_key_up(tinyrl, key);
		break;
	case tinyrl_vt100_CURSOR_DOWN:
		result = tinyrl_key_down(tinyrl, key);
		break;
	case tinyrl_vt100_CURSOR_LEFT:
		result = tinyrl_key_left(tinyrl, key);
		break;
	case tinyrl_vt100_CURSOR_RIGHT:
		result = tinyrl_key_right(tinyrl, key);
		break;
	case tinyrl_vt100_HOME:
		result = tinyrl_key_start_of_line(tinyrl,key);
		break;
	case tinyrl_vt100_END:
		result = tinyrl_key_end_of_line(tinyrl,key);
		break;
	case tinyrl_vt100_DELETE:
		result = tinyrl_key_delete(tinyrl,key);
		break;
	case tinyrl_vt100_INSERT:
	case tinyrl_vt100_PGDOWN:
	case tinyrl_vt100_PGUP:
	case tinyrl_vt100_UNKNOWN:
		break;
	}

	return result;
}

/*-------------------------------------------------------- */
int tinyrl_printf(const tinyrl_t * tinyrl, const char *fmt, ...)
{
	va_list args;
	int len;

	va_start(args, fmt);
	len = tinyrl_vt100_vprintf(tinyrl->term, fmt, args);
	va_end(args);

	return len;
}


/*----------------------------------------------------------------------- */
static void tinyrl_internal_print(const tinyrl_t * tinyrl, const char *text)
{
	if (tinyrl->echo_enabled) {
		/* simply echo the line */
		tinyrl_vt100_printf(tinyrl->term, "%s", text);
	} else {
		/* replace the line with echo char if defined */
		if (tinyrl->echo_char) {
			unsigned int i = strlen(text);
			while (i--) {
				tinyrl_vt100_printf(tinyrl->term, "%c",
					tinyrl->echo_char);
			}
		}
	}
}

/*----------------------------------------------------------------------- */
static void tinyrl_internal_position(const tinyrl_t *tinyrl, int prompt_len,
	int line_len, unsigned int width)
{
	int rows, cols;

	rows = ((line_len + prompt_len) / width) - (prompt_len / width);
	cols = ((line_len + prompt_len) % width) - (prompt_len % width);
	if (cols > 0)
		tinyrl_vt100_cursor_back(tinyrl->term, cols);
	else if (cols < 0)
		tinyrl_vt100_cursor_forward(tinyrl->term, -cols);
	if (rows > 0)
		tinyrl_vt100_cursor_up(tinyrl->term, rows);
	else if (rows < 0)
		tinyrl_vt100_cursor_down(tinyrl->term, -rows);
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
void tinyrl_redisplay(tinyrl_t * tinyrl)
{
	unsigned int line_size = strlen(tinyrl->line);
	unsigned int line_len = utf8_nsyms(tinyrl, tinyrl->line, line_size);
	unsigned int width = tinyrl_vt100__get_width(tinyrl->term);
	unsigned int count, eq_chars = 0;
	int cols;

	/* Prepare print position */
	if (tinyrl->last_buffer && (width == tinyrl->width)) {
		unsigned int eq_len = 0;
		/* If line and last line have the equal chars at begining */
		eq_chars = lub_string_equal_part(tinyrl->line, tinyrl->last_buffer,
			tinyrl->utf8);
		eq_len = utf8_nsyms(tinyrl, tinyrl->last_buffer, eq_chars);
		count = utf8_nsyms(tinyrl, tinyrl->last_buffer, tinyrl->last_point);
		tinyrl_internal_position(tinyrl, tinyrl->prompt_len + eq_len,
			count - eq_len, width);
	} else {
		/* Prepare to resize */
		if (width != tinyrl->width) {
			tinyrl_vt100_next_line(tinyrl->term);
			tinyrl_vt100_erase_down(tinyrl->term);
		}
		tinyrl_vt100_printf(tinyrl->term, "%s", tinyrl->prompt);
	}

	/* Print current line */
	tinyrl_internal_print(tinyrl, tinyrl->line + eq_chars);
	cols = (tinyrl->prompt_len + line_len) % width;
	if (!cols && (line_size - eq_chars))
		tinyrl_vt100_next_line(tinyrl->term);
	/* Erase down if current line is shorter than previous one */
	if (tinyrl->last_line_size > line_size)
		tinyrl_vt100_erase_down(tinyrl->term);
	/* Move the cursor to the insertion point */
	if (tinyrl->point < line_size) {
		unsigned int pre_len = utf8_nsyms(tinyrl,
			tinyrl->line, tinyrl->point);
		count = utf8_nsyms(tinyrl, tinyrl->line + tinyrl->point,
			line_size - tinyrl->point);
		tinyrl_internal_position(tinyrl, tinyrl->prompt_len + pre_len,
			count, width);
	}

	/* Update the display */
	tinyrl_vt100_oflush(tinyrl->term);

	/* Save the last line buffer */
	lub_string_free(tinyrl->last_buffer);
	tinyrl->last_buffer = lub_string_dup(tinyrl->line);
	tinyrl->last_point = tinyrl->point;
	tinyrl->width = width;
	tinyrl->last_line_size = line_size;
}


static char *internal_insertline(tinyrl_t * tinyrl, char *buffer)
{
	char *p;
	char *s = buffer;

	/* strip any spurious '\r' or '\n' */
	if ((p = strchr(buffer, '\r')))
		*p = '\0';
	if ((p = strchr(buffer, '\n')))
		*p = '\0';
	/* skip any whitespace at the beginning of the line */
	if (0 == tinyrl->point) {
		while (*s && isspace(*s))
			s++;
	}
	if (*s) {
		/* append tinyrl string to the input buffer */
		(void)tinyrl_insert_text(tinyrl, s);
	}
	/* echo the command to the output stream */
	tinyrl_redisplay(tinyrl);

	return s;
}

/*----------------------------------------------------------------------- */
static char *internal_readline(tinyrl_t * tinyrl,
	void *context, const char *str)
{
	FILE *istream = tinyrl_vt100__get_istream(tinyrl->term);
	char *result = NULL;
	int lerrno = 0;

	tinyrl->done = BOOL_FALSE;
	tinyrl->point = 0;
	tinyrl->end = 0;
	tinyrl->buffer = lub_string_dup("");
	tinyrl->buffer_size = strlen(tinyrl->buffer);
	tinyrl->line = tinyrl->buffer;
	tinyrl->context = context;

	/* Interactive session */
	if (isatty(fileno(tinyrl->istream)) && !str) {
		unsigned int utf8_cont = 0; /* UTF-8 continue bytes */
		unsigned int esc_cont = 0; /* Escape sequence continues */
		char esc_seq[10]; /* Buffer for ESC sequence */
		char *esc_p = esc_seq;

		/* Set the terminal into raw mode */
		tty_raw_mode(tinyrl);
		tinyrl_reset_line_state(tinyrl);

		while (!tinyrl->done) {
			int key;

			key = tinyrl_getchar(tinyrl);

			/* Error || EOF || Timeout */
			if (key < 0) {
				if ((VT100_TIMEOUT == key) &&
					!tinyrl->timeout_fn(tinyrl))
					continue;
				/* It's time to finish the session */
				tinyrl->done = BOOL_TRUE;
				tinyrl->line = NULL;
				lerrno = ENOENT;
				continue;
			}

			/* Real key pressed */
			/* Common callback for any key */
			if (tinyrl->keypress_fn)
				tinyrl->keypress_fn(tinyrl, key);

			/* Check for ESC sequence. It's a special case. */
			if (!esc_cont && (key == KEY_ESC)) {
				esc_cont = 1; /* Start ESC sequence */
				esc_p = esc_seq;
				continue;
			}
			if (esc_cont) {
				/* Broken sequence */
				if (esc_p >= (esc_seq + sizeof(esc_seq) - 1)) {
					esc_cont = 0;
					continue;
				}
				/* Dump the control sequence into sequence buffer
				   ANSI standard control sequences will end
				   with a character between 64 - 126 */
				*esc_p = key & 0xff;
				esc_p++;
				/* tinyrl is an ANSI control sequence terminator code */
				if ((key != '[') && (key > 63)) {
					*esc_p = '\0';
					tinyrl_escape_seq(tinyrl, esc_seq);
					esc_cont = 0;
					tinyrl_redisplay(tinyrl);
				}
				continue;
			}

			/* Call the handler for tinyrl key */
			if (!tinyrl->handlers[key](tinyrl, key))
				tinyrl_ding(tinyrl);
			if (tinyrl->done) /* Some handler set the done flag */
				continue; /* It will break the loop */

			if (tinyrl->utf8) {
				if (!(UTF8_7BIT_MASK & key)) /* ASCII char */
					utf8_cont = 0;
				else if (utf8_cont && (UTF8_10 == (key & UTF8_MASK))) /* Continue byte */
					utf8_cont--;
				else if (UTF8_11 == (key & UTF8_MASK)) { /* First byte of multibyte char */
					/* Find out number of char's bytes */
					int b = key;
					utf8_cont = 0;
					while ((utf8_cont < 6) && (UTF8_10 != (b & UTF8_MASK))) {
						utf8_cont++;
						b = b << 1;
					}
				}
			}
			/* For non UTF-8 encoding the utf8_cont is always 0.
			   For UTF-8 it's 0 when one-byte symbol or we get
			   all bytes for the current multibyte character. */
			if (!utf8_cont)
				tinyrl_redisplay(tinyrl);
		}
		/* If the last character in the line (other than NULL)
		   is a space remove it. */
		if (tinyrl->end && tinyrl->line && isspace(tinyrl->line[tinyrl->end - 1]))
			tinyrl_delete_text(tinyrl, tinyrl->end - 1, tinyrl->end);
		/* Restores the terminal mode */
		tty_restore_mode(tinyrl);

	/* Non-interactive session */
	} else {
		char *s = NULL, buffer[80];
		size_t len = sizeof(buffer);
		char *tmp = NULL;

		/* manually reset the line state without redisplaying */
		lub_string_free(tinyrl->last_buffer);
		tinyrl->last_buffer = NULL;

		if (str) {
			tmp = lub_string_dup(str);
			internal_insertline(tinyrl, tmp);
		} else {
			while (istream && (sizeof(buffer) == len) &&
				(s = fgets(buffer, sizeof(buffer), istream))) {
				s = internal_insertline(tinyrl, buffer);
				len = strlen(buffer) + 1; /* account for the '\0' */
			}
			if (!s || ((tinyrl->line[0] == '\0') && feof(istream))) {
				/* time to finish the session */
				tinyrl->line = NULL;
				lerrno = ENOENT;
			}
		}

		/*
		 * check against fgets returning null as either error or end of file.
		 * tinyrl is a measure to stop potential task spin on encountering an
		 * error from fgets.
		 */
		if (tinyrl->line && !tinyrl->handlers[KEY_LF](tinyrl, KEY_LF)) {
			/* an issue has occured */
			tinyrl->line = NULL;
			lerrno = ENOEXEC;
		}
		if (str)
			lub_string_free(tmp);
	}
	/*
	 * duplicate the string for return to the client 
	 * we have to duplicate as we may be referencing a
	 * history entry or our internal buffer
	 */
	result = tinyrl->line ? lub_string_dup(tinyrl->line) : NULL;

	/* free our internal buffer */
	free(tinyrl->buffer);
	tinyrl->buffer = NULL;

	if (!result)
		errno = lerrno; /* get saved errno */
	return result;
}

/*----------------------------------------------------------------------- */
char *tinyrl_readline(tinyrl_t * tinyrl, void *context)
{
	return internal_readline(tinyrl, context, NULL);
}

/*----------------------------------------------------------------------- */
char *tinyrl_forceline(tinyrl_t * tinyrl, void *context, const char *line)
{
	return internal_readline(tinyrl, context, line);
}

/*----------------------------------------------------------------------- */
/*
 * Ensure that buffer has enough space to hold len characters,
 * possibly reallocating it if necessary. The function returns BOOL_TRUE
 * if the line is successfully extended, BOOL_FALSE if not.
 */
bool_t tinyrl_extend_line_buffer(tinyrl_t * tinyrl, unsigned int len)
{
	bool_t result = BOOL_TRUE;
	char *new_buffer;
	size_t new_len = len;

	if (tinyrl->buffer_size >= len)
		return result;

	/*
	 * What we do depends on whether we are limited by
	 * memory or a user imposed limit.
	 */
	if (tinyrl->max_line_length == 0) {
		/* make sure we don't realloc too often */
		if (new_len < tinyrl->buffer_size + 10)
			new_len = tinyrl->buffer_size + 10;
		/* leave space for terminator */
		new_buffer = realloc(tinyrl->buffer, new_len + 1);

		if (!new_buffer) {
			tinyrl_ding(tinyrl);
			result = BOOL_FALSE;
		} else {
			tinyrl->buffer_size = new_len;
			tinyrl->line = tinyrl->buffer = new_buffer;
		}
	} else {
		if (new_len < tinyrl->max_line_length) {

			/* Just reallocate once to the max size */
			new_buffer = realloc(tinyrl->buffer,
				tinyrl->max_line_length);

			if (!new_buffer) {
				tinyrl_ding(tinyrl);
				result = BOOL_FALSE;
			} else {
				tinyrl->buffer_size =
					tinyrl->max_line_length - 1;
				tinyrl->line = tinyrl->buffer = new_buffer;
			}
		} else {
			tinyrl_ding(tinyrl);
			result = BOOL_FALSE;
		}
	}

	return result;
}

/*----------------------------------------------------------------------- */
/*
 * Insert text into the line at the current cursor position.
 */
bool_t tinyrl_insert_text(tinyrl_t * tinyrl, const char *text)
{
	unsigned int delta = strlen(text);

	/*
	 * If the client wants to change the line ensure that the line and buffer
	 * references are in sync
	 */
	changed_line(tinyrl);

	if ((delta + tinyrl->end) > (tinyrl->buffer_size)) {
		/* extend the current buffer */
		if (BOOL_FALSE ==
			tinyrl_extend_line_buffer(tinyrl, tinyrl->end + delta))
			return BOOL_FALSE;
	}

	if (tinyrl->point < tinyrl->end) {
		/* move the current text to the right (including the terminator) */
		memmove(&tinyrl->buffer[tinyrl->point + delta],
			&tinyrl->buffer[tinyrl->point],
			(tinyrl->end - tinyrl->point) + 1);
	} else {
		/* terminate the string */
		tinyrl->buffer[tinyrl->end + delta] = '\0';
	}

	/* insert the new text */
	strncpy(&tinyrl->buffer[tinyrl->point], text, delta);

	/* now update the indexes */
	tinyrl->point += delta;
	tinyrl->end += delta;

	return BOOL_TRUE;
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

/*----------------------------------------------------------------------- */
/*
 * Delete the text between start and end in the current line. (inclusive)
 * tinyrl adjusts the rl_point and rl_end indexes appropriately.
 */
void tinyrl_delete_text(tinyrl_t * tinyrl, unsigned int start, unsigned int end)
{
	unsigned int delta;

	/*
	 * If the client wants to change the line ensure that the line and buffer
	 * references are in sync
	 */
	changed_line(tinyrl);

	/* make sure we play it safe */
	if (start > end) {
		unsigned int tmp = end;
		start = end;
		end = tmp;
	}
	if (end > tinyrl->end)
		end = tinyrl->end;

	delta = (end - start) + 1;

	/* move any text which is left */
	memmove(&tinyrl->buffer[start],
		&tinyrl->buffer[start + delta], tinyrl->end - end);

	/* now adjust the indexs */
	if (tinyrl->point >= start) {
		if (tinyrl->point > end) {
			/* move the insertion point back appropriately */
			tinyrl->point -= delta;
		} else {
			/* move the insertion point to the start */
			tinyrl->point = start;
		}
	}
	if (tinyrl->end > end)
		tinyrl->end -= delta;
	else
		tinyrl->end = start;
	/* put a terminator at the end of the buffer */
	tinyrl->buffer[tinyrl->end] = '\0';
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
void tinyrl__set_hotkey_fn(tinyrl_t *tinyrl,
	tinyrl_key_func_t *fn)
{
	tinyrl->hotkey_fn = fn;
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
