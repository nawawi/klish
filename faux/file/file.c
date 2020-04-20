/** @file file.c
 * @brief Functions for working with files.
 *
 * This library was created to exclude glibc's file stream operations like
 * fopen(), fgets() etc. These functions use glibc internal buffer. To work
 * with buffer glibc has its own fflush() function and special behaviour while
 * fclose(). It brings a problems with stream file objects and system file
 * descriptors while fork(). The file streams and system file descriptors can't
 * be used interchangeably. So faux file library uses standard system file
 * operations like open(), read() and emulate some usefull stream function like
 * getline(). The faux file object has own buffer and doesn't use glibc's one.
 * The faux_file_close() doesn't lseek() file descriptor as fclose() can do.
 * You can use faux file object and standard file operation in the same time.
 * The only thing to remember is internal buffer that can contain already
 * readed bytes.
 */

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include "private.h"
#include "faux/faux.h"
#include "faux/file.h"
#include "faux/str.h"


/** @brief Create file object using existent fd.
 *
 * Create file object an link it to existent file descriptor.
 *
 * @return Allocated and initialized file object or NULL on error.
 */
faux_file_t *faux_file_fdopen(int fd) {

	struct stat stat_struct = {};
	faux_file_t *f = NULL;

	// Before object creation check is fd valid.
	// Try to get stat().
	if (fstat(fd, &stat_struct) < 0)
		return NULL; // Illegal fd

	f = faux_zmalloc(sizeof(*f));
	assert(f);
	if (!f)
		return NULL;

	// Init
	f->fd = fd;
	f->buf_size = FAUX_FILE_CHUNK_SIZE;
	f->buf = faux_zmalloc(f->buf_size);
	assert(f->buf);
	if (!f->buf) {
		faux_free(f);
		return NULL;
	}
	f->len = 0;

	return f;
}


faux_file_t *faux_file_open(const char *pathname, int flags, mode_t mode) {

	int fd = -1;

	assert(pathname);
	if (!pathname)
		return NULL;

	fd = open(pathname, flags, mode);
	if (fd < 0)
		return NULL;

	return faux_file_fdopen(fd);
}


int faux_file_close(faux_file_t *f) {

	int fd = -1;

	assert(f);
	if (!f)
		return -1;

	fd = f->fd;
	faux_free(f->buf);
	faux_free(f);

	return close(fd);
}


int faux_file_fileno(faux_file_t *f) {

	assert(f);
	if (!f)
		return -1;

	return f->fd;
}


static char *faux_file_takeaway_line(faux_file_t *f) {

	char *find = NULL;
	const char *eol = "\n\r";
	size_t line_len = 0;
	char *line = NULL;

	assert(f);
	if (!f)
		return NULL;

	find = faux_str_charsn(f->buf, eol, f->len);
	if (!find)
		return NULL; // End of line is not found
	line_len = find - f->buf;
	line = faux_str_dupn(f->buf, line_len);
	assert(line);
	if (!line)
		return NULL; // Memory problems

	// Remove line from the internal buffer
	// Remove EOL char also. So additional '1' is used
	f->len = f->len - line_len - 1;
	memmove(f->buf, find + 1, f->len);

	return line;
}

char *faux_file_getline(faux_file_t *f) {

	char *find = NULL;

	assert(f);
	if (!f)
		return NULL;

	// May be buffer already contain line
	find = faux_file_takeaway_line(f);
	if (find)
		return find;

	return NULL;
}
