#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <errno.h>

#include <tinyrl/vt100.h>


struct vt100_s {
	FILE *istream;
	FILE *ostream;
};


typedef struct {
	const char *sequence;
	vt100_esc_e code;
} vt100_decode_t;


// This table maps the vt100 escape codes to an enumeration
static vt100_decode_t esc_map[] = {
	{"[A", VT100_CURSOR_UP},
	{"[B", VT100_CURSOR_DOWN},
	{"[C", VT100_CURSOR_RIGHT},
	{"[D", VT100_CURSOR_LEFT},
	{"[H", VT100_HOME},
	{"[1~", VT100_HOME},
	{"[F", VT100_END},
	{"[4~", VT100_END},
	{"[2~", VT100_INSERT},
	{"[3~", VT100_DELETE},
	{"[5~", VT100_PGUP},
	{"[6~", VT100_PGDOWN},
};


// Hotkeys
const char *hotkey_map[] = {
	"^@", /* 0 Null character */
	"^A", /* 1 Start of heading, = console interrupt */
	"^B", /* 2 Start of text, maintenance mode on HP console */
	"^C", /* 3 End of text */
	"^D", /* 4 End of transmission, not the same as ETB */
	"^E", /* 5 Enquiry, goes with ACK; old HP flow control */
	"^F", /* 6 Acknowledge, clears ENQ logon hand */
	"^G", /* 7 Bell, rings the bell... */
	"^H", /* 8 Backspace, works on HP terminals/computers */
	"^I", /* 9 Horizontal tab, move to next tab stop */
	"^J", /* 10 Line Feed */
	"^K", /* 11 Vertical tab */
	"^L", /* 12 Form Feed, page eject */
	"^M", /* 13 Carriage Return*/
	"^N", /* 14 Shift Out, alternate character set */
	"^O", /* 15 Shift In, resume defaultn character set */
	"^P", /* 16 Data link escape */
	"^Q", /* 17 XON, with XOFF to pause listings; "okay to send". */
	"^R", /* 18 Device control 2, block-mode flow control */
	"^S", /* 19 XOFF, with XON is TERM=18 flow control */
	"^T", /* 20 Device control 4 */
	"^U", /* 21 Negative acknowledge */
	"^V", /* 22 Synchronous idle */
	"^W", /* 23 End transmission block, not the same as EOT */
	"^X", /* 24 Cancel line, MPE echoes !!! */
	"^Y", /* 25 End of medium, Control-Y interrupt */
	"^Z", /* 26 Substitute */
	"^[", /* 27 Escape, next character is not echoed */
	"^\\", /* 28 File separator */
	"^]", /* 29 Group separator */
	"^^", /* 30 Record separator, block-mode terminator */
	"^_",  /* 31 Unit separator */
	NULL
};


vt100_t *vt100_new(FILE *istream, FILE *ostream)
{
	vt100_t *vt100 = NULL;

	vt100 = malloc(sizeof(vt100_t));
	if (!vt100)
		return NULL;

	// Initialize
	vt100->istream = istream;
	vt100->ostream = ostream;

	return vt100;
}


void vt100_free(vt100_t *vt100)
{
	free(vt100);
}


FILE *vt100_istream(const vt100_t *vt100)
{
	if (!vt100)
		return NULL;

	return vt100->istream;
}


void vt100_set_istream(vt100_t *vt100, FILE *istream)
{
	if (!vt100)
		return;

	vt100->istream = istream;
}


FILE *vt100_ostream(const vt100_t *vt100)
{
	if (!vt100)
		return NULL;

	return vt100->ostream;
}


void vt100_set_ostream(vt100_t *vt100, FILE *ostream)
{
	if (!vt100)
		return;

	vt100->ostream = ostream;
}


vt100_esc_e vt100_esc_decode(const char *esc_seq)
{
	vt100_esc_e result = VT100_UNKNOWN;
	unsigned int i = 0;

	for (i = 0; i < (sizeof(esc_map) / sizeof(vt100_decode_t)); i++) {
		if (strcmp(esc_map[i].sequence, esc_seq))
			continue;
		result = esc_map[i].code;
		break;
	}

	return result;
}


ssize_t vt100_hotkey_decode(const char *hotkey)
{
	unsigned int i = 0;

	if (!hotkey)
		return -1;

	while (hotkey_map[i]) {
		if (strcmp(hotkey_map[i], hotkey) == 0)
			return i;
		i++;
	}

	return -1;
}


int vt100_printf(const vt100_t *vt100, const char *fmt, ...)
{
	va_list args;
	int len = 0;

	// If ostream is not set don't consider it as a error. Consider it
	// as a printf to NULL.
	if (!vt100 || !vt100->ostream)
		return 0;

	va_start(args, fmt);
	len = vt100_vprintf(vt100, fmt, args);
	va_end(args);

	return len;
}


int vt100_vprintf(const vt100_t *vt100, const char *fmt, va_list args)
{
	if (!vt100 || !vt100->ostream)
		return 0;

	return vfprintf(vt100->ostream, fmt, args);
}


int vt100_getchar(const vt100_t *vt100, unsigned char *c)
{
	if (!vt100 || !vt100->istream || !c) {
		errno = ENOENT;
		return -1;
	}

	return read(fileno(vt100->istream), c, 1);
}


int vt100_oflush(const vt100_t *vt100)
{
	if (!vt100 || !vt100->ostream)
		return 0;

	return fflush(vt100->ostream);
}


int vt100_ierror(const vt100_t *vt100)
{
	if (!vt100 || !vt100->istream)
		return 0;

	return ferror(vt100->istream);
}


int vt100_oerror(const vt100_t *vt100)
{
	if (!vt100 || !vt100->ostream)
		return 0;

	return ferror(vt100->ostream);
}


int vt100_ieof(const vt100_t *vt100)
{
	if (!vt100 || !vt100->istream)
		return 0;

	return feof(vt100->istream);
}


int vt100_eof(const vt100_t *vt100)
{
	if (!vt100 || !vt100->istream)
		return 0;

	return feof(vt100->istream);
}


void vt100_winsize(const vt100_t *vt100, size_t *width, size_t *height)
{
#ifdef TIOCGWINSZ
	struct winsize ws = {};
	int res = 0;
#endif
	size_t w = 80;
	size_t h = 25;

#ifdef TIOCGWINSZ
	if(vt100 && vt100->ostream && isatty(fileno(vt100->ostream))) {
		res = ioctl(fileno(vt100->ostream), TIOCGWINSZ, &ws);
		if (!res) {
			if (0 != ws.ws_col)
				w = (size_t)ws.ws_col;
			if (0 != ws.ws_row)
				h = (size_t)ws.ws_row;
		}
	}
#endif

	if (width)
		*width = w;
	if (height)
		*height = h;
}

size_t vt100_width(const vt100_t *vt100)
{
	size_t w = 0;

	vt100_winsize(vt100, &w, NULL);

	return w;
}


size_t vt100_height(const vt100_t *vt100)
{
	size_t h = 0;

	vt100_winsize(vt100, NULL, &h);

	return h;
}


void vt100_ding(const vt100_t *vt100)
{
	vt100_printf(vt100, "%c", KEY_BEL);
	vt100_oflush(vt100);
}


void vt100_attr_reset(const vt100_t *vt100)
{
	vt100_printf(vt100, "%c[0m", KEY_ESC);
}


void vt100_attr_bright(const vt100_t *vt100)
{
	vt100_printf(vt100, "%c[1m", KEY_ESC);
}


void vt100_attr_dim(const vt100_t *vt100)
{
	vt100_printf(vt100, "%c[2m", KEY_ESC);
}


void vt100_attr_underscore(const vt100_t *vt100)
{
	vt100_printf(vt100, "%c[4m", KEY_ESC);
}


void vt100_attr_blink(const vt100_t *vt100)
{
	vt100_printf(vt100, "%c[5m", KEY_ESC);
}


void vt100_attr_reverse(const vt100_t *vt100)
{
	vt100_printf(vt100, "%c[7m", KEY_ESC);
}


void vt100_attr_hidden(const vt100_t *vt100)
{
	vt100_printf(vt100, "%c[8m", KEY_ESC);
}


void vt100_erase_line(const vt100_t *vt100)
{
	vt100_printf(vt100, "%c[2K", KEY_ESC);
}


void vt100_clear_screen(const vt100_t *vt100)
{
	vt100_printf(vt100, "%c[2J", KEY_ESC);
}


void vt100_cursor_save(const vt100_t *vt100)
{
	vt100_printf(vt100, "%c7", KEY_ESC); // VT100
//	vt100_printf(vt100, "%c[s", KEY_ESC); // ANSI
}


void vt100_cursor_restore(const vt100_t *vt100)
{
	vt100_printf(vt100, "%c8", KEY_ESC); // VT100
//	vt100_printf(vt100, "%c[u", KEY_ESC); // ANSI
}


void vt100_cursor_forward(const vt100_t *vt100, size_t count)
{
	vt100_printf(vt100, "%c[%dC", KEY_ESC, count);
}


void vt100_cursor_back(const vt100_t *vt100, size_t count)
{
	vt100_printf(vt100, "%c[%dD", KEY_ESC, count);
}


void vt100_cursor_up(const vt100_t *vt100, size_t count)
{
	vt100_printf(vt100, "%c[%dA", KEY_ESC, count);
}


void vt100_cursor_down(const vt100_t *vt100, size_t count)
{
	vt100_printf(vt100, "%c[%dB", KEY_ESC, count);
}


void vt100_scroll_up(const vt100_t *vt100)
{
	vt100_printf(vt100, "%cD", KEY_ESC);
}


void vt100_scroll_down(const vt100_t *vt100)
{
	vt100_printf(vt100, "%cM", KEY_ESC);
}


void vt100_next_line(const vt100_t *vt100)
{
	vt100_printf(vt100, "%cE", KEY_ESC);
}

void vt100_cursor_home(const vt100_t *vt100)
{
	vt100_printf(vt100, "%c[H", KEY_ESC);
}


void vt100_erase(const vt100_t *vt100, size_t count)
{
	vt100_printf(vt100, "%c[%dP", KEY_ESC, count);
}


void vt100_erase_down(const vt100_t *vt100)
{
	vt100_printf(vt100, "%c[J", KEY_ESC);
}
