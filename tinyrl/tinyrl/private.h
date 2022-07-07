#include <termios.h>

#include <faux/faux.h>

#include "tinyrl/vt100.h"
#include "tinyrl/hist.h"
#include "tinyrl/tinyrl.h"


// UTF-8 functions
ssize_t utf8_to_wchar(const char *sp, unsigned long *sym_out);
bool_t utf8_wchar_is_cjk(unsigned long sym);
char *utf8_move_left(const char *line, char *cur_pos);
char *utf8_move_right(const char *line, char *cur_pos);

// Keys
bool_t tinyrl_key_default(tinyrl_t * tinyrl, char key);
bool_t tinyrl_key_interrupt(tinyrl_t * tinyrl, char key);
bool_t tinyrl_key_start_of_line(tinyrl_t * tinyrl, char key);
bool_t tinyrl_key_end_of_line(tinyrl_t * tinyrl, char key);
bool_t tinyrl_key_kill(tinyrl_t * tinyrl, char key);
bool_t tinyrl_key_yank(tinyrl_t * tinyrl, char key);
bool_t tinyrl_key_crlf(tinyrl_t * tinyrl, char key);
bool_t tinyrl_key_up(tinyrl_t * tinyrl, char key);
bool_t tinyrl_key_down(tinyrl_t * tinyrl, char key);
bool_t tinyrl_key_left(tinyrl_t * tinyrl, char key);
bool_t tinyrl_key_right(tinyrl_t * tinyrl, char key);
bool_t tinyrl_key_backspace(tinyrl_t *tinyrl, char key);
bool_t tinyrl_key_backword(tinyrl_t *tinyrl, char key);
bool_t tinyrl_key_delete(tinyrl_t * tinyrl, char key);
bool_t tinyrl_key_clear_screen(tinyrl_t * tinyrl, char key);
bool_t tinyrl_key_erase_line(tinyrl_t * tinyrl, char key);
bool_t tinyrl_key_tab(tinyrl_t * tinyrl, char key);

// Tinyrl
bool_t tinyrl_line_extend(tinyrl_t *tinyrl, size_t len);
bool_t tinyrl_line_insert(tinyrl_t *tinyrl, const char *text, size_t len);
bool_t tinyrl_esc_seq(tinyrl_t *tinyrl, const char *esc_seq);


typedef struct line_s {
	char *str;
	size_t size; // Size of buffer
	size_t len; // Length of string
	off_t pos;
} line_t;

#define NUM_HANDLERS 256

struct tinyrl_s {

	tinyrl_key_func_t *handlers[NUM_HANDLERS]; // Handlers for pressed keys
	tinyrl_key_func_t *hotkey_fn; // Handler for non-standard hotkeys
	hist_t *hist; // History object
	vt100_t *term; // VT100 terminal object
	struct termios default_termios; // Saved terminal settings
	unsigned int width; // Terminal width
	bool_t utf8; // Is encoding UTF-8 flag. Default is UTF-8
	line_t line; // Current line

	// Input processing vars. Input is processed char by char so
	// the current state of processing is necessary.
	size_t utf8_cont; //  Number of UTF-8 continue bytes left
	bool_t esc_cont; // Does escape sequence continue
	char esc_seq[10]; // Current ESC sequence (line doesn't contain it)
	char *esc_p; // Pointer for unfinished ESC sequence


	unsigned max_line_length;
	char *prompt;
	size_t prompt_size; /* strlen() */
	size_t prompt_len; /* Symbol positions */
	char *buffer;
	size_t buffer_size;
	bool_t done;
	bool_t completion_over;
	bool_t completion_error_over;
	tinyrl_completion_func_t *attempted_completion_function;
	int state;
#define RL_STATE_COMPLETING (0x00000001)
	char *kill_string;
	void *context;		/* context supplied by caller
				 * to tinyrl_readline()
				 */
	char echo_char;
	bool_t echo_enabled;
	char *last_buffer;	/* hold record of the previous
				buffer for redisplay purposes */
	unsigned int last_point; /* hold record of the previous
				cursor position for redisplay purposes */
	unsigned int last_line_size; /* The length of last_buffer */
};
