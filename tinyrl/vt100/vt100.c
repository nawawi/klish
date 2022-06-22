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

struct vt100 {
	FILE *istream;
	FILE *ostream;
	int   timeout; /* Input timeout in seconds */
};

#include "private.h"

typedef struct {
	const char* sequence;
	vt100_escape_e code;
} vt100_decode_t;

/* This table maps the vt100 escape codes to an enumeration */
static vt100_decode_t cmds[] = {
	{"[A", vt100_CURSOR_UP},
	{"[B", vt100_CURSOR_DOWN},
	{"[C", vt100_CURSOR_RIGHT},
	{"[D", vt100_CURSOR_LEFT},
	{"[H", vt100_HOME},
	{"[1~", vt100_HOME},
	{"[F", vt100_END},
	{"[4~", vt100_END},
	{"[2~", vt100_INSERT},
	{"[3~", vt100_DELETE},
	{"[5~", vt100_PGUP},
	{"[6~", vt100_PGDOWN},
};

/*--------------------------------------------------------- */
vt100_escape_e vt100_escape_decode(const vt100_t *this,
	const char *esc_seq)
{
	vt100_escape_e result = vt100_UNKNOWN;
	unsigned int i;

	/* Decode the sequence to macros */
	for (i = 0; i < (sizeof(cmds) / sizeof(vt100_decode_t)); i++) {
		if (strcmp(cmds[i].sequence, esc_seq))
			continue;
		result = cmds[i].code;
		break;
	}

	this = this; /* Happy compiler */

	return result;
}

/*-------------------------------------------------------- */
int vt100_printf(const vt100_t * this, const char *fmt, ...)
{
	va_list args;
	int len;

	if (!this->ostream)
		return 0;
	va_start(args, fmt);
	len = vt100_vprintf(this, fmt, args);
	va_end(args);

	return len;
}

/*-------------------------------------------------------- */
int
vt100_vprintf(const vt100_t * this, const char *fmt, va_list args)
{
	if (!this->ostream)
		return 0;
	return vfprintf(this->ostream, fmt, args);
}

/*-------------------------------------------------------- */
int vt100_getchar(const vt100_t *this)
{
	unsigned char c;
	int istream_fd;
	fd_set rfds;
	struct timeval tv;
	int retval;
	ssize_t res;

	if (!this->istream)
		return VT100_ERR;
	istream_fd = fileno(this->istream);

	/* Just wait for the input if no timeout */
	if (this->timeout <= 0) {
		while (((res = read(istream_fd, &c, 1)) < 0) &&
			(EAGAIN == errno));
		/* EOF or error */
		if (res < 0)
			return VT100_ERR;
		if (!res)
			return VT100_EOF;
		return c;
	}

	/* Set timeout for the select() */
	FD_ZERO(&rfds);
	FD_SET(istream_fd, &rfds);
	tv.tv_sec = this->timeout;
	tv.tv_usec = 0;
	while (((retval = select(istream_fd + 1, &rfds, NULL, NULL, &tv)) < 0) &&
		(EAGAIN == errno));
	/* Error or timeout */
	if (retval < 0)
		return VT100_ERR;
	if (!retval)
		return VT100_TIMEOUT;

	res = read(istream_fd, &c, 1);
	/* EOF or error */
	if (res < 0)
		return VT100_ERR;
	if (!res)
		return VT100_EOF;

	return c;
}

/*-------------------------------------------------------- */
int vt100_oflush(const vt100_t * this)
{
	if (!this->ostream)
		return 0;
	return fflush(this->ostream);
}

/*-------------------------------------------------------- */
int vt100_ierror(const vt100_t * this)
{
	if (!this->istream)
		return 0;
	return ferror(this->istream);
}

/*-------------------------------------------------------- */
int vt100_oerror(const vt100_t * this)
{
	if (!this->ostream)
		return 0;
	return ferror(this->ostream);
}

/*-------------------------------------------------------- */
int vt100_ieof(const vt100_t * this)
{
	if (!this->istream)
		return 0;
	return feof(this->istream);
}

/*-------------------------------------------------------- */
int vt100_eof(const vt100_t * this)
{
	if (!this->istream)
		return 0;
	return feof(this->istream);
}

/*-------------------------------------------------------- */
unsigned int vt100__get_width(const vt100_t *this)
{
#ifdef TIOCGWINSZ
	struct winsize ws;
	int res;
#endif

	if(!this->ostream)
		return 80;

#ifdef TIOCGWINSZ
	ws.ws_col = 0;
	res = ioctl(fileno(this->ostream), TIOCGWINSZ, &ws);
	if (res || !ws.ws_col)
		return 80;
	return ws.ws_col;
#else
	return 80;
#endif
}

/*-------------------------------------------------------- */
unsigned int vt100__get_height(const vt100_t *this)
{
#ifdef TIOCGWINSZ
	struct winsize ws;
	int res;
#endif

	if(!this->ostream)
		return 25;

#ifdef TIOCGWINSZ
	ws.ws_row = 0;
	res = ioctl(fileno(this->ostream), TIOCGWINSZ, &ws);
	if (res || !ws.ws_row)
		return 25;
	return ws.ws_row;
#else
	return 25;
#endif
}

/*-------------------------------------------------------- */
static void
vt100_init(vt100_t * this, FILE * istream, FILE * ostream)
{
	this->istream = istream;
	this->ostream = ostream;
	this->timeout = -1; /* No timeout by default */
}

/*-------------------------------------------------------- */
static void vt100_fini(vt100_t * this)
{
	/* nothing to do yet... */
	this = this;
}

/*-------------------------------------------------------- */
vt100_t *vt100_new(FILE * istream, FILE * ostream)
{
	vt100_t *this = NULL;

	this = malloc(sizeof(vt100_t));
	if (this) {
		vt100_init(this, istream, ostream);
	}

	return this;
}

/*-------------------------------------------------------- */
void vt100_delete(vt100_t * this)
{
	vt100_fini(this);
	/* release the memory */
	free(this);
}

/*-------------------------------------------------------- */
void vt100_ding(const vt100_t * this)
{
	vt100_printf(this, "%c", KEY_BEL);
	(void)vt100_oflush(this);
}

/*-------------------------------------------------------- */
void vt100_attribute_reset(const vt100_t * this)
{
	vt100_printf(this, "%c[0m", KEY_ESC);
}

/*-------------------------------------------------------- */
void vt100_attribute_bright(const vt100_t * this)
{
	vt100_printf(this, "%c[1m", KEY_ESC);
}

/*-------------------------------------------------------- */
void vt100_attribute_dim(const vt100_t * this)
{
	vt100_printf(this, "%c[2m", KEY_ESC);
}

/*-------------------------------------------------------- */
void vt100_attribute_underscore(const vt100_t * this)
{
	vt100_printf(this, "%c[4m", KEY_ESC);
}

/*-------------------------------------------------------- */
void vt100_attribute_blink(const vt100_t * this)
{
	vt100_printf(this, "%c[5m", KEY_ESC);
}

/*-------------------------------------------------------- */
void vt100_attribute_reverse(const vt100_t * this)
{
	vt100_printf(this, "%c[7m", KEY_ESC);
}

/*-------------------------------------------------------- */
void vt100_attribute_hidden(const vt100_t * this)
{
	vt100_printf(this, "%c[8m", KEY_ESC);
}

/*-------------------------------------------------------- */
void vt100_erase_line(const vt100_t * this)
{
	vt100_printf(this, "%c[2K", KEY_ESC);
}

/*-------------------------------------------------------- */
void vt100_clear_screen(const vt100_t * this)
{
	vt100_printf(this, "%c[2J", KEY_ESC);
}

/*-------------------------------------------------------- */
void vt100_cursor_save(const vt100_t * this)
{
	vt100_printf(this, "%c7", KEY_ESC); /* VT100 */
/*	vt100_printf(this, "%c[s", KEY_ESC); */ /* ANSI */
}

/*-------------------------------------------------------- */
void vt100_cursor_restore(const vt100_t * this)
{
	vt100_printf(this, "%c8", KEY_ESC); /* VT100 */
/*	vt100_printf(this, "%c[u", KEY_ESC); */ /* ANSI */
}

/*-------------------------------------------------------- */
void vt100_cursor_forward(const vt100_t * this, unsigned count)
{
	vt100_printf(this, "%c[%dC", KEY_ESC, count);
}

/*-------------------------------------------------------- */
void vt100_cursor_back(const vt100_t * this, unsigned count)
{
	vt100_printf(this, "%c[%dD", KEY_ESC, count);
}

/*-------------------------------------------------------- */
void vt100_cursor_up(const vt100_t * this, unsigned count)
{
	vt100_printf(this, "%c[%dA", KEY_ESC, count);
}

/*-------------------------------------------------------- */
void vt100_cursor_down(const vt100_t * this, unsigned count)
{
	vt100_printf(this, "%c[%dB", KEY_ESC, count);
}

/*-------------------------------------------------------- */
void vt100_scroll_up(const vt100_t *this)
{
	vt100_printf(this, "%cD", KEY_ESC);
}

/*-------------------------------------------------------- */
void vt100_scroll_down(const vt100_t *this)
{
	vt100_printf(this, "%cM", KEY_ESC);
}

/*-------------------------------------------------------- */
void vt100_next_line(const vt100_t *this)
{
	vt100_printf(this, "%cE", KEY_ESC);
}

/*-------------------------------------------------------- */
void vt100_cursor_home(const vt100_t * this)
{
	vt100_printf(this, "%c[H", KEY_ESC);
}

/*-------------------------------------------------------- */
void vt100_erase(const vt100_t * this, unsigned count)
{
	vt100_printf(this, "%c[%dP", KEY_ESC, count);
}

/*-------------------------------------------------------- */
void vt100__set_timeout(vt100_t *this, int timeout)
{
	this->timeout = timeout;
}

/*-------------------------------------------------------- */
void vt100_erase_down(const vt100_t * this)
{
	vt100_printf(this, "%c[J", KEY_ESC);
}

/*-------------------------------------------------------- */
void vt100__set_istream(vt100_t * this, FILE * istream)
{
	this->istream = istream;
}

/*-------------------------------------------------------- */
FILE *vt100__get_istream(const vt100_t * this)
{
	return this->istream;
}

/*-------------------------------------------------------- */
FILE *vt100__get_ostream(const vt100_t * this)
{
	return this->ostream;
}

/*-------------------------------------------------------- */
