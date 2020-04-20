#include "faux/faux.h"
#include "faux/file.h"

/** @brief Chunk size to allocate buffer */
#define FAUX_FILE_CHUNK_SIZE 128

struct faux_file_s {
	int fd; // File descriptor
	char *buf; // Data buffer
	size_t buf_size; // Current buffer size
	size_t len; // Current data length
};
