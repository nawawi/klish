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


bool_t tinyrl_key_default(tinyrl_t *tinyrl, unsigned char key)
{
	if (key > 31) {
		// Inject new char to the line
		tinyrl_line_insert(tinyrl, (const char *)(&key), 1);
	} else {
		// Call the external hotkey analyzer
		if (tinyrl->hotkey_fn)
			tinyrl->hotkey_fn(tinyrl, key);
	}

	return BOOL_TRUE;
}


bool_t tinyrl_key_interrupt(tinyrl_t *tinyrl, unsigned char key)
{
	tinyrl_multi_crlf(tinyrl);
	tinyrl_reset_line_state(tinyrl);
	tinyrl_reset_line(tinyrl);
	tinyrl_reset_hist_pos(tinyrl);

	// Happy compiler
	key = key;

	return BOOL_TRUE;
}


bool_t tinyrl_key_start_of_line(tinyrl_t *tinyrl, unsigned char key)
{
	// Set current position to the start of the line
	tinyrl->line.pos = 0;

	// Happy compiler
	key = key;

	return BOOL_TRUE;
}


bool_t tinyrl_key_end_of_line(tinyrl_t *tinyrl, unsigned char key)
{
	// Set current position to the end of the line
	tinyrl->line.pos = tinyrl->line.len;

	// Happy compiler
	key = key;

	return BOOL_TRUE;
}


bool_t tinyrl_key_kill(tinyrl_t *tinyrl, unsigned char key)
{
	// Free old buffered string
	faux_str_free(tinyrl->buffer);
	tinyrl->buffer = NULL;

	// Nothing to kill
	if (tinyrl->line.pos == tinyrl->line.len)
		return BOOL_TRUE;
	// Store killed string
	tinyrl->buffer = faux_str_dup(tinyrl->line.str + tinyrl->line.pos);
	// Delete text to the end of the line
	tinyrl_line_delete(tinyrl, tinyrl->line.pos, tinyrl->line.len);

	// Happy compiler
	key = key;

	return BOOL_TRUE;
}


bool_t tinyrl_key_yank(tinyrl_t *tinyrl, unsigned char key)
{
	if (!tinyrl->buffer)
		return BOOL_TRUE;

	tinyrl_line_insert(tinyrl, tinyrl->buffer, strlen(tinyrl->buffer));

	// Happy compiler
	key = key;

	return BOOL_TRUE;
}


// Default handler for crlf
bool_t tinyrl_key_crlf(tinyrl_t *tinyrl, unsigned char key)
{

	tinyrl_multi_crlf(tinyrl);
	tinyrl_reset_line_state(tinyrl);
	tinyrl_reset_line(tinyrl);
	tinyrl_reset_hist_pos(tinyrl);

	key = key; // Happy compiler

	return BOOL_TRUE;
}


bool_t tinyrl_key_up(tinyrl_t *tinyrl, unsigned char key)
{
	const char *str = NULL;

	str = hist_pos(tinyrl->hist);
	// Up key is pressed for the first time and current line is new typed one
	if (!str) {
		hist_add(tinyrl->hist, tinyrl->line.str, BOOL_TRUE); // Temp entry
		hist_pos_up(tinyrl->hist); // Skip newly added temp entry
	}
	hist_pos_up(tinyrl->hist);
	str = hist_pos(tinyrl->hist);
	// Empty history
	if (!str)
		return BOOL_TRUE;
	tinyrl_line_replace(tinyrl, str);

	key = key; // Happy compiler

	return BOOL_TRUE;
}


bool_t tinyrl_key_down(tinyrl_t *tinyrl, unsigned char key)
{
	const char *str = NULL;

	hist_pos_down(tinyrl->hist);
	str = hist_pos(tinyrl->hist);
	// Empty history
	if (!str)
		return BOOL_TRUE;
	tinyrl_line_replace(tinyrl, str);

	key = key; // Happy compiler

	return BOOL_TRUE;
}


bool_t tinyrl_key_left(tinyrl_t *tinyrl, unsigned char key)
{
	if (tinyrl->line.pos == 0)
		return BOOL_TRUE;

	if (tinyrl->utf8)
		tinyrl->line.pos = utf8_move_left(tinyrl->line.str, tinyrl->line.pos);
	else
		tinyrl->line.pos--;

	// Happy compiler
	key = key;

	return BOOL_TRUE;
}


bool_t tinyrl_key_right(tinyrl_t *tinyrl, unsigned char key)
{
	if (tinyrl->line.pos == tinyrl->line.len)
		return BOOL_TRUE;

	if (tinyrl->utf8)
		tinyrl->line.pos = utf8_move_right(tinyrl->line.str, tinyrl->line.pos);
	else
		tinyrl->line.pos++;

	// Happy compiler
	key = key;

	return BOOL_TRUE;
}


bool_t tinyrl_key_backspace(tinyrl_t *tinyrl, unsigned char key)
{
	if (tinyrl->line.pos == 0)
		return BOOL_TRUE;

	if (tinyrl->utf8) {
		off_t new_pos = 0;
		new_pos = utf8_move_left(tinyrl->line.str, tinyrl->line.pos);
		tinyrl_line_delete(tinyrl, new_pos, tinyrl->line.pos - new_pos);
	} else {
		tinyrl_line_delete(tinyrl, tinyrl->line.pos - 1, 1);
	}

	// Happy compiler
	key = key;

	return BOOL_TRUE;
}


bool_t tinyrl_key_delete(tinyrl_t *tinyrl, unsigned char key)
{
	if (tinyrl->line.pos == tinyrl->line.len)
		return BOOL_TRUE;

	if (tinyrl->utf8) {
		off_t new_pos = 0;
		new_pos = utf8_move_right(tinyrl->line.str, tinyrl->line.pos);
		tinyrl_line_delete(tinyrl, tinyrl->line.pos, new_pos - tinyrl->line.pos);
	} else {
		tinyrl_line_delete(tinyrl, tinyrl->line.pos, 1);
	}

	// Happy compiler
	key = key;

	return BOOL_TRUE;
}


bool_t tinyrl_key_backword(tinyrl_t *tinyrl, unsigned char key)
{
	size_t new_pos = tinyrl->line.pos;

	// Free old buffered string
	faux_str_free(tinyrl->buffer);
	tinyrl->buffer = NULL;

	if (tinyrl->line.pos == 0)
		return BOOL_TRUE;

	// Remove spaces before cursor
	while (new_pos > 0) {
		size_t prev_pos = tinyrl->utf8 ?
			utf8_move_left(tinyrl->line.str, new_pos) : (new_pos - 1);
		if (!isspace(tinyrl->line.str[prev_pos]))
			break;
		new_pos = prev_pos;
	}

	// Delete word before cusor
	while (new_pos > 0) {
		size_t prev_pos = tinyrl->utf8 ?
			utf8_move_left(tinyrl->line.str, new_pos) : (new_pos - 1);
		if (isspace(tinyrl->line.str[prev_pos]))
			break;
		new_pos = prev_pos;
	}

	if (new_pos == tinyrl->line.pos)
		return BOOL_TRUE;

	// Store string
	tinyrl->buffer = faux_str_dupn(tinyrl->line.str + new_pos,
		tinyrl->line.pos - new_pos);
	tinyrl_line_delete(tinyrl, new_pos, tinyrl->line.pos - new_pos);

	// Happy compiler
	key = key;

	return BOOL_TRUE;
}


bool_t tinyrl_key_clear_screen(tinyrl_t *tinyrl, unsigned char key)
{
	vt100_clear_screen(tinyrl->term);
	vt100_cursor_home(tinyrl->term);
	tinyrl_reset_line_state(tinyrl);

	key = key; // Happy compiler

	return BOOL_TRUE;
}


bool_t tinyrl_key_erase_line(tinyrl_t *tinyrl, unsigned char key)
{
	// Free old buffered string
	faux_str_free(tinyrl->buffer);
	tinyrl->buffer = NULL;

	// Nothing to erase
	if (tinyrl->line.len == 0)
		return BOOL_TRUE;
	// Store string
	tinyrl->buffer = faux_str_dup(tinyrl->line.str);
	// Delete text to the end of the line
	tinyrl_line_delete(tinyrl, 0, tinyrl->line.len);

	// Happy compiler
	key = key;

	return BOOL_TRUE;
}


// Key tab handler is needed to mask real <tab> output
bool_t tinyrl_key_tab(tinyrl_t *tinyrl, unsigned char key)
{
	// Happy compiler
	tinyrl = tinyrl;
	key = key;

	return BOOL_TRUE;
}
