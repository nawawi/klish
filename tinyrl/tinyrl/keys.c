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


bool_t tinyrl_key_default(tinyrl_t * tinyrl, int key)
{
	bool_t result = BOOL_FALSE;
/*
	if (key > 31) {
		char tmp[2];
		tmp[0] = (key & 0xFF), tmp[1] = '\0';
		// inject tinyrl text into the buffer 
		result = tinyrl_insert_text(tinyrl, tmp);
	} else {
		// Call the external hotkey analyzer 
		if (tinyrl->hotkey_fn)
			tinyrl->hotkey_fn(tinyrl, key);
	}
*/
	return result;
}


bool_t tinyrl_key_interrupt(tinyrl_t * tinyrl, int key)
{
/*
	tinyrl_crlf(tinyrl);
	tinyrl_delete_text(tinyrl, 0, tinyrl->end);
	tinyrl->done = BOOL_TRUE;
	// keep the compiler happy 
	key = key;
*/
	return BOOL_TRUE;
}


bool_t tinyrl_key_start_of_line(tinyrl_t * tinyrl, int key)
{
/*
	// set the insertion point to the start of the line 
	tinyrl->point = 0;
	// keep the compiler happy 
	key = key;
*/
	return BOOL_TRUE;
}


bool_t tinyrl_key_end_of_line(tinyrl_t * tinyrl, int key)
{
/*
	// set the insertion point to the end of the line 
	tinyrl->point = tinyrl->end;
	// keep the compiler happy 
	key = key;
*/
	return BOOL_TRUE;
}


bool_t tinyrl_key_kill(tinyrl_t * tinyrl, int key)
{
/*
	// release any old kill string 
	lub_string_free(tinyrl->kill_string);

	// store the killed string 
	tinyrl->kill_string = lub_string_dup(&tinyrl->buffer[tinyrl->point]);

	// delete the text to the end of the line 
	tinyrl_delete_text(tinyrl, tinyrl->point, tinyrl->end);
	// keep the compiler happy 
	key = key;
*/
	return BOOL_TRUE;
}


bool_t tinyrl_key_yank(tinyrl_t * tinyrl, int key)
{
	bool_t result = BOOL_FALSE;
/*
	if (tinyrl->kill_string) {
		// insert the kill string at the current insertion point 
		result = tinyrl_insert_text(tinyrl, tinyrl->kill_string);
	}
	// keep the compiler happy 
	key = key;
*/
	return result;
}


bool_t tinyrl_key_crlf(tinyrl_t * tinyrl, int key)
{
/*
	tinyrl_crlf(tinyrl);
	tinyrl->done = BOOL_TRUE;
	// keep the compiler happy 
	key = key;
*/
	return BOOL_TRUE;
}


bool_t tinyrl_key_up(tinyrl_t * tinyrl, int key)
{
	bool_t result = BOOL_FALSE;
/*
	tinyrl_history_entry_t *entry = NULL;
	if (tinyrl->line == tinyrl->buffer) {
		// go to the last history entry 
		entry = tinyrl_history_getlast(tinyrl->history, &tinyrl->hist_iter);
	} else {
		// already traversing the history list so get previous 
		entry = tinyrl_history_getprevious(&tinyrl->hist_iter);
	}
	if (entry) {
		// display the entry moving the insertion point
		 * to the end of the line 
		 
		tinyrl->line = tinyrl_history_entry__get_line(entry);
		tinyrl->point = tinyrl->end = strlen(tinyrl->line);
		result = BOOL_TRUE;
	}
	// keep the compiler happy 
	key = key;
*/
	return result;
}


bool_t tinyrl_key_down(tinyrl_t * tinyrl, int key)
{
	bool_t result = BOOL_FALSE;
/*
	if (tinyrl->line != tinyrl->buffer) {
		// we are not already at the bottom 
		// the iterator will have been set up by the key_up() function 
		tinyrl_history_entry_t *entry =
		    tinyrl_history_getnext(&tinyrl->hist_iter);
		if (!entry) {
			// nothing more in the history list 
			tinyrl->line = tinyrl->buffer;
		} else {
			tinyrl->line = tinyrl_history_entry__get_line(entry);
		}
		// display the entry moving the insertion point
		// to the end of the line 
		tinyrl->point = tinyrl->end = strlen(tinyrl->line);
		result = BOOL_TRUE;
	}
	// keep the compiler happy 
	key = key;
*/
	return result;
}


bool_t tinyrl_key_left(tinyrl_t * tinyrl, int key)
{
	bool_t result = BOOL_FALSE;
/*
	if (tinyrl->point > 0) {
		tinyrl->point--;
		utf8_point_left(tinyrl);
		result = BOOL_TRUE;
	}
	// keep the compiler happy 
	key = key;
*/
	return result;
}


bool_t tinyrl_key_right(tinyrl_t * tinyrl, int key)
{
	bool_t result = BOOL_FALSE;
/*
	if (tinyrl->point < tinyrl->end) {
		tinyrl->point++;
		utf8_point_right(tinyrl);
		result = BOOL_TRUE;
	}
	// keep the compiler happy 
	key = key;
*/
	return result;
}


bool_t tinyrl_key_backspace(tinyrl_t *tinyrl, int key)
{
	bool_t result = BOOL_FALSE;
/*
	if (tinyrl->point) {
		unsigned int end = --tinyrl->point;
		utf8_point_left(tinyrl);
		tinyrl_delete_text(tinyrl, tinyrl->point, end);
		result = BOOL_TRUE;
	}
	// keep the compiler happy 
	key = key;
*/
	return result;
}


bool_t tinyrl_key_backword(tinyrl_t *tinyrl, int key)
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

	// keep the compiler happy 
	key = key;
*/
	return result;
}

bool_t tinyrl_key_delete(tinyrl_t * tinyrl, int key)
{
	bool_t result = BOOL_FALSE;
/*
	if (tinyrl->point < tinyrl->end) {
		unsigned int begin = tinyrl->point++;
		utf8_point_right(tinyrl);
		tinyrl_delete_text(tinyrl, begin, tinyrl->point - 1);
		result = BOOL_TRUE;
	}
	// keep the compiler happy 
	key = key;
*/
	return result;
}


bool_t tinyrl_key_clear_screen(tinyrl_t * tinyrl, int key)
{
/*
	tinyrl_vt100_clear_screen(tinyrl->term);
	tinyrl_vt100_cursor_home(tinyrl->term);
	tinyrl_reset_line_state(tinyrl);

	// keep the compiler happy 
	key = key;
	tinyrl = tinyrl;
*/
	return BOOL_TRUE;
}


bool_t tinyrl_key_erase_line(tinyrl_t * tinyrl, int key)
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

	// keep the compiler happy 
	key = key;
	tinyrl = tinyrl;
*/
	return BOOL_TRUE;
}


bool_t tinyrl_key_tab(tinyrl_t * tinyrl, int key)
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
	// keep the compiler happy 
	key = key;
*/
	return result;
}
