#ifndef _konf_buf_private_h
#define _konf_buf_private_h

#include "konf/buf.h"

struct konf_buf_s {
	int fd;
	int size;
	char *buf;
	int pos;
	int rpos;
	void *data; /* Optional pointer to arbitrary related data */
};

#endif
