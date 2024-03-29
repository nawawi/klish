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
/*
	// release any old kill string 
	lub_string_free(tinyrl->kill_string);

	// store the killed string 
	tinyrl->kill_string = lub_string_dup(&tinyrl->buffer[tinyrl->point]);

	// delete the text to the end of the line 
	tinyrl_delete_text(tinyrl, tinyrl->point, tinyrl->end);
*/
	// Happy compiler
	tinyrl = tinyrl;
	key = key;

	return BOOL_TRUE;
}


bool_t tinyrl_key_yank(tinyrl_t *tinyrl, unsigned char key)
{
	bool_t result = BOOL_FALSE;
/*
	if (tinyrl->kill_string) {
		// insert the kill string at the current insertion point 
		result = tinyrl_insert_text(tinyrl, tinyrl->kill_string);
	}
*/
	// Happy compiler
	tinyrl = tinyrl;
	key = key;

	return result;
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
	bool_t result = BOOL_FALSE;
/*
    // remove current whitespace before cursor 
	while (tinyrl->point > 0 && isspace(tinyrl->line[tinyrl->point - 1]))
        tinyrl_key_backspace(tinyrl, KEY_BS);

    // delete word before cusor 
	while (tinyrl->point > 0 && !isspace(tinyrl->line[tinyrl->point - 1]))
        tinyrl_key_backspace(tinyrl, KEY_BS);

	result = BOOL_TRUE;
*/
	// Happy compiler
	tinyrl = tinyrl;
	key = key;

	return result;
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
/*	unsigned int end;

	// release any old kill string 
	lub_string_free(tinyrl->kill_string);

	if (!tinyrl->point) {
		tinyrl->kill_string = NULL;
		return BOOL_TRUE;
	}

	end = tinyrl->point - 1;

	// store the killed string 
	tinyrl->kill_string = malloc(tinyrl->point + 1);
	memcpy(tinyrl->kill_string, tinyrl->buffer, tinyrl->point);
	tinyrl->kill_string[tinyrl->point] = '\0';

	// delete the text from the start of the line 
	tinyrl_delete_text(tinyrl, 0, end);
	tinyrl->point = 0;
*/
	// Happy compiler
	tinyrl = tinyrl;
	key = key;

	return BOOL_TRUE;
}


bool_t tinyrl_key_tab(tinyrl_t *tinyrl, unsigned char key)
{
	bool_t result = BOOL_FALSE;
/*
	tinyrl_match_e status = tinyrl_complete_with_extensions(tinyrl);

	switch (status) {
	case TINYRL_COMPLETED_MATCH:
	case TINYRL_MATCH:
		// everything is OK with the world... 
		result = tinyrl_insert_text(tinyrl, " ");
		break;
	case TINYRL_NO_MATCH:
	case TINYRL_MATCH_WITH_EXTENSIONS:
	case TINYRL_AMBIGUOUS:
	case TINYRL_COMPLETED_AMBIGUOUS:
		// oops don't change the result and let the bell ring 
		break;
	}
*/
	// Happy compiler
	tinyrl = tinyrl;
	key = key;

	return result;
}
