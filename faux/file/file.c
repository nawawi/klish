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
 * Create file object and link it to existent file descriptor.
 *
 * @param [in] fd Already opened file descriptor.
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


/** @brief Create file object and open correspondent file.
 *
 * Function opens specified file using flags and create file object based
 * on this opened file. The object must be freed and file must be closed
 * later by the faux_file_close().
 *
 * @warning The faux_file_close() must be executed later to free file object.
 *
 * @param [in] pathname File name.
 * @param [in] flags Flags to open file (like O_RDONLY etc).
 * @param [in] mode File permissions if file will be created.
 * @return File object or NULL on error.
 */
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


/** @brief Closes file and frees file object.
 *
 * Function closes previously opened (by faux_file_open() or faux_file_fdopen())
 * file and frees file object structures.
 *
 * @param [in] f File object to close and free.
 * @return 0 - success, < 0 - error
 */
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


/** @brief Returns file descriptor from file object.
 *
 * Works like fileno() function for stream objects.
 *
 * @param [in] f File object.
 * @return Linked file descriptor.
 */
int faux_file_fileno(faux_file_t *f) {

	assert(f);
	if (!f)
		return -1;

	return f->fd;
}


/** @brief Returns EOF flag.
 *
 * @param [in] f File object
 * @return BOOL_TRUE if it's end of file and BOOL_FALSE else.
 */
bool_t faux_file_eof(const faux_file_t *f) {

	assert(f);
	if (!f)
		return BOOL_FALSE;

	return f->eof;
}


/** @brief Service static function to take away data block from internal buffer.
 *
 * Returns allocated data block in a form of C-string i.e. adds '\0' at the end
 * of data. Additionally function can drop some bytes from internal buffer.
 * It's usefull when it's necessary to get text string from the buffer and
 * drop trailing end of line.
 *
 * @warning Returned pointer must be freed by faux_str_free() later.
 *
 * @param [in] f File object.
 * @param [in] bytes_get Number of bytes to get from internal buffer.
 * @param [in] bytes_drop Number of bytes to drop. Actually
 * (bytes_drop + bytes_get) bytes will be removed from internal buffer.
 * @return Allocated string (with trailing '\0') with data to get.
 */
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


/** @brief Service static function to get all data from buf as single C-string.
 *
 * Gets all the data from internal buffer as a single C-string (i.e. ends with
 * '\0'). This data will be removed from internal buffer.
 *
 * @warning Returned pointer must be freed by faux_str_free() later.
 *
 * @param [in] f File object.
 * @return Allocated string (with trailing '\0') with data to get.
 */
static char *faux_file_takeaway_rest(faux_file_t *f) {

	assert(f);
	if (!f)
		return NULL;

	return faux_file_takeaway(f, f->len, 0);
}


/** @brief Service static function to get line from buf as single C-string.
 *
 * Gets line (data ends with EOL) from internal buffer as a single C-string
 * (i.e. ends with '\0'). The resulting line will not contain trailing EOL but
 * EOL will be removed from internal buffer together with line.
 *
 * @warning Returned pointer must be freed by faux_str_free() later.
 *
 * @param [in] f File object.
 * @return Allocated string (with trailing '\0') with line.
 */
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


/** @brief Service static function to enlarge internal buffer.
 *
 * The initial size of internal buffer is 128 bytes. Each function execution
 * enlarges buffer by chunk of 128 bytes.
 *
 * @param [in] f File objects.
 * @return 0 - success, < 0 - error
 */
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


/** @brief Read line from file.
 *
 * Actually function searches for line within internal buffer. If line is not
 * found then function reads new data from file and searches for the line again.
 * The last line in file (without trailing EOL) is considered as line too.
 *
 * @warning Returned pointer must be freed by faux_str_free() later.
 *
 * @param [in] f File object.
 * @return Line pointer or NULL on error.
 */
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


/** @brief Writes data to file.
 *
 * The system write() can be interrupted by signal or can write less bytes
 * than specified. This function will continue to write data until all data
 * will be written or error occured.
 *
 * @param [in] f File object.
 * @param [in] buf Buffer to write.
 * @param [in] n Number of bytes to write.
 * @return Number of bytes written or NULL on error.
 */
ssize_t faux_file_write(faux_file_t *f, const void *buf, size_t n) {

	ssize_t bytes_written = 0;
	size_t left = n;
	const void *data = buf;

	assert(f);
	assert(buf);
	if (!f || !buf)
		return -1;
	if (0 == n)
		return 0;

	do {
		bytes_written = write(f->fd, data, left);
		if (bytes_written < 0) {
			if (EINTR == errno)
				continue;
			return -1;
		}
		if (0 == bytes_written) // Insufficient space
			return -1;
		data += bytes_written;
		left = left - bytes_written;
	} while (left > 0);

	return n;
}
