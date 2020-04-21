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
#include <errno.h>

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
	f->eof = BOOL_FALSE;

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


bool_t faux_file_eof(const faux_file_t *f) {

	assert(f);
	if (!f)
		return BOOL_FALSE;

	return f->eof;
}


static char *faux_file_takeaway(faux_file_t *f,
	size_t bytes_get, size_t bytes_drop) {

	size_t remove_len = 0;
	char *line = NULL;

	assert(f);
	if (!f)
		return NULL;

	remove_len = bytes_get + bytes_drop;
	// Try to take away more bytes than buffer contain
	if ((remove_len > f->len) || (0 == remove_len))
		return NULL;

	line = faux_zmalloc(bytes_get + 1); // One extra byte for '\0'
	assert(line);
	if (!line)
		return NULL; // Memory problems
	memcpy(line, f->buf, bytes_get);

	// Remove block from the internal buffer
	f->len = f->len - remove_len;
	memmove(f->buf, f->buf + remove_len, f->len);

	return line;
}


static char *faux_file_takeaway_rest(faux_file_t *f) {

	assert(f);
	if (!f)
		return NULL;

	return faux_file_takeaway(f, f->len, 0);
}


static char *faux_file_takeaway_line(faux_file_t *f) {

	char *find = NULL;
	const char *eol = "\n\r";
	size_t line_len = 0;

	assert(f);
	if (!f)
		return NULL;

	// Search buffer for EOL
	find = faux_str_charsn(f->buf, eol, f->len);
	if (!find)
		return NULL; // End of line is not found
	line_len = find - f->buf;

	// Takeaway line without trailing EOL. So drop one last byte
	return faux_file_takeaway(f, line_len, 1);
}


static int faux_file_enlarge_buffer(faux_file_t *f) {

	size_t new_size = 0;
	char *new_buf = NULL;

	assert(f);
	if (!f)
		return -1;

	new_size = f->buf_size + FAUX_FILE_CHUNK_SIZE;
	new_buf = realloc(f->buf, new_size);
	assert(new_buf);
	if (!new_buf)
		return -1;
	// NULLify newly allocated memory
	faux_bzero(new_buf + f->buf_size, new_size - f->buf_size);
	f->buf = new_buf;
	f->buf_size = new_size;

	return 0;
}


char *faux_file_getline(faux_file_t *f) {

	ssize_t bytes_readed = 0;

	assert(f);
	if (!f)
		return NULL;

	do {
		char *find = NULL;

		// May be buffer already contain line
		find = faux_file_takeaway_line(f);
		if (find)
			return find;

		if (f->buf_size == f->len) { // Buffer is full but doesn't contain line
			if (faux_file_enlarge_buffer(f) < 0) // Make buffer larger
				return NULL; // Memory problem
		}

		// Read new data from file
		do {
			bytes_readed = read(f->fd, f->buf + f->len, f->buf_size - f->len);
			if ((bytes_readed < 0) && (errno != EINTR))
				return NULL; // Some file error
		} while (bytes_readed < 0); // i.e. EINTR
		f->len += bytes_readed;

	} while (bytes_readed > 0);

	// EOF (here bytes_readed == 0)
	f->eof = BOOL_TRUE;

	// The last line can be without eol. Consider it as a line too
	return faux_file_takeaway_rest(f);
}
