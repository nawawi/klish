/** @file vt100.h
 *
 */

#ifndef _tinyrl_vt100_h
#define _tinyrl_vt100_h

#include <stdio.h>
#include <stdarg.h>

#include <faux/faux.h>


typedef struct vt100_s vt100_t;


// Key codes
#define KEY_NUL	0	// ^@ Null character
#define KEY_SOH	1	// ^A Start of heading, = console interrupt
#define KEY_STX	2	// ^B Start of text, maintenance mode on HP console
#define KEY_ETX	3	// ^C End of text
#define KEY_EOT	4	// ^D End of transmission, not the same as ETB
#define KEY_ENQ	5	// ^E Enquiry, goes with ACK; old HP flow control
#define KEY_ACK	6	// ^F Acknowledge, clears ENQ logon hand
#define KEY_BEL	7	// ^G Bell, rings the bell
#define KEY_BS	8	// ^H Backspace, works on HP terminals/computers
#define KEY_HT	9	// ^I Horizontal tab, move to next tab stop
#define KEY_LF	10	// ^J Line Feed
#define KEY_VT	11	// ^K Vertical tab
#define KEY_FF	12	// ^L Form Feed, page eject
#define KEY_CR	13	// ^M Carriage Return
#define KEY_SO	14	// ^N Shift Out, alternate character set
#define KEY_SI	15	// ^O Shift In, resume defaultn character set
#define KEY_DLE	16	// ^P Data link escape
#define KEY_DC1	17	// ^Q XON, with XOFF to pause listings; "okay to send"
#define KEY_DC2	18	// ^R Device control 2, block-mode flow control
#define KEY_DC3	19	// ^S XOFF, with XON is TERM=18 flow control
#define KEY_DC4	20	// ^T Device control 4
#define KEY_NAK	21	// ^U Negative acknowledge
#define KEY_SYN	22	// ^V Synchronous idle
#define KEY_ETB	23	// ^W End transmission block, not the same as EOT
#define KEY_CAN	24	// ^X Cancel line, MPE echoes !!!
#define KEY_EM	25	// ^Y End of medium, Control-Y interrupt
#define KEY_SUB	26	// ^Z Substitute
#define KEY_ESC	27	// ^[ Escape, next character is not echoed
#define KEY_FS	28	// ^\ File separator
#define KEY_GS	29	// ^] Group separator
#define KEY_RS	30	// ^^ Record separator, block-mode terminator
#define KEY_US	31	// ^_ Unit separator
#define KEY_DEL	127	// Delete (not a real control character)


// Types of escape code
typedef enum {
	VT100_UNKNOWN,		// Undefined escape sequence
	VT100_CURSOR_UP,	// Move the cursor up
	VT100_CURSOR_DOWN,	// Move the cursor down
	VT100_CURSOR_LEFT,	// Move the cursor left
	VT100_CURSOR_RIGHT,	// Move the cursor right
	VT100_HOME,		// Move the cursor to the beginning of the line
	VT100_END,		// Move the cursor to the end of the line
	VT100_INSERT,		// No action at the moment
	VT100_DELETE,		// Delete character on the right
	VT100_PGUP,		// No action at the moment
	VT100_PGDOWN		// No action at the moment
} vt100_esc_e;


C_DECL_BEGIN

vt100_t *vt100_new(FILE *istream, FILE *ostream);
void vt100_free(vt100_t *vt100);

FILE *vt100_istream(const vt100_t *vt100);
void vt100_set_istream(vt100_t *vt100, FILE *istream);
FILE *vt100_ostream(const vt100_t *vt100);
void vt100_set_ostream(vt100_t *vt100, FILE *ostream);

size_t vt100_width(const vt100_t *vt100);
size_t vt100_height(const vt100_t *vt100);

int vt100_printf(const vt100_t *vt100, const char *fmt, ...);
int vt100_vprintf(const vt100_t *vt100, const char *fmt, va_list args);
int vt100_oflush(const vt100_t *vt100);
int vt100_ierror(const vt100_t *vt100);
int vt100_oerror(const vt100_t *vt100);
int vt100_ieof(const vt100_t *vt100);
int vt100_getchar(const vt100_t *vt100, char *c);
vt100_esc_e vt100_esc_decode(const vt100_t *vt100, const char *esc_seq);

void vt100_ding(const vt100_t *vt100);
void vt100_attr_reset(const vt100_t *vt100);
void vt100_attr_bright(const vt100_t *vt100);
void vt100_attr_dim(const vt100_t *vt100);
void vt100_attr_underscore(const vt100_t *vt100);
void vt100_attr_blink(const vt100_t *vt100);
void vt100_attr_reverse(const vt100_t *vt100);
void vt100_attr_hidden(const vt100_t *vt100);
void vt100_erase_line(const vt100_t *vt100);
void vt100_clear_screen(const vt100_t *vt100);
void vt100_cursor_back(const vt100_t *vt100, size_t count);
void vt100_cursor_forward(const vt100_t *vt100, size_t count);
void vt100_cursor_up(const vt100_t *vt100, size_t count);
void vt100_cursor_down(const vt100_t *vt100, size_t count);
void vt100_scroll_up(const vt100_t *instance);
void vt100_scroll_down(const vt100_t *instance);
void vt100_next_line(const vt100_t *instance);
void vt100_cursor_home(const vt100_t *vt100);
void vt100_cursor_save(const vt100_t *vt100);
void vt100_cursor_restore(const vt100_t *vt100);
void vt100_erase(const vt100_t *vt100, size_t count);
void vt100_erase_down(const vt100_t *vt100);

C_DECL_END

#endif // _tinyrl_vt100_h
