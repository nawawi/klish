/*
 * buf.c
 *
 * This file provides the implementation of a buf class
 */
#include "private.h"
#include "lub/argv.h"
#include "lub/string.h"
#include "lub/ctype.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#define KONF_BUF_CHUNK 1024

/*-------------------------------------------------------- */
int konf_buf_compare(const void *first, const void *second)
{
	const konf_buf_t *f = (const konf_buf_t *)first;
	const konf_buf_t *s = (const konf_buf_t *)second;
	return (f->fd - s->fd);
}

/*-------------------------------------------------------- */
static void konf_buf_init(konf_buf_t * this, int fd)
{
	this->fd = fd;
	this->buf = malloc(KONF_BUF_CHUNK);
	this->size = KONF_BUF_CHUNK;
	this->pos = 0;
	this->rpos = 0;
	this->data = NULL;
}

/*--------------------------------------------------------- */
static void konf_buf_fini(konf_buf_t * this)
{
	free(this->buf);
}

/*--------------------------------------------------------- */
konf_buf_t *konf_buf_new(int fd)
{
	konf_buf_t *this = malloc(sizeof(konf_buf_t));

	if (this)
		konf_buf_init(this, fd);

	return this;
}

/*-------------------------------------------------------- */
void konf_buf_delete(void *data)
{
	konf_buf_t *this = (konf_buf_t *)data;
	konf_buf_fini(this);
	free(this);
}

/*--------------------------------------------------------- */
static int konf_buf_realloc(konf_buf_t *this, int addsize)
{
	int chunk = KONF_BUF_CHUNK;
	char *tmpbuf;

	if (addsize > chunk)
		chunk = addsize;
	if ((this->size - this->pos) < chunk) {
		tmpbuf = realloc(this->buf, this->size + chunk);
		this->buf = tmpbuf;
		this->size += chunk;
	}

	return this->size;
}

/*--------------------------------------------------------- */
int konf_buf_add(konf_buf_t *this, void *str, size_t len)
{
	char *buffer;

	konf_buf_realloc(this, len);
	buffer = this->buf + this->pos;
	memcpy(buffer, str, len);
	this->pos += len;

	return len;
}

/*--------------------------------------------------------- */
int konf_buf_read(konf_buf_t *this)
{
	char *buffer;
	int buffer_size;
	int nbytes;

	konf_buf_realloc(this, 0);
	buffer_size = this->size - this->pos;
	buffer = this->buf + this->pos;

	nbytes = read(this->fd, buffer, buffer_size);
	if (nbytes > 0)
		this->pos += nbytes;

	return nbytes;
}

/*--------------------------------------------------------- */
char * konf_buf_string(char *buf, int len)
{
	int i;
	char *str;

	for (i = 0; i < len; i++) {
		if (('\0' == buf[i]) ||
			('\n' == buf[i]))
			break;
	}
	if (i >= len)
		return NULL;

	str = malloc(i + 1);
	memcpy(str, buf, i + 1);
	str[i] = '\0';

	return str;
}

/*--------------------------------------------------------- */
char * konf_buf_parse(konf_buf_t *this)
{
	char * str = NULL;

	/* Search the buffer for the string */
	str = konf_buf_string(this->buf, this->pos);

	/* Remove parsed string from the buffer */
	if (str) {
		int len = strlen(str) + 1;
		memmove(this->buf, &this->buf[len], this->pos - len);
		this->pos -= len;
		if (this->rpos >= len)
			this->rpos -= len;
		else
			this->rpos = 0;
	}

	/* Make buffer shorter */
	if ((this->size - this->pos) > (2 * KONF_BUF_CHUNK)) {
		char *tmpbuf;
		tmpbuf = realloc(this->buf, this->size - KONF_BUF_CHUNK);
		this->buf = tmpbuf;
		this->size -= KONF_BUF_CHUNK;
	}

	return str;
}

/*--------------------------------------------------------- */
char * konf_buf_preparse(konf_buf_t *this)
{
	char * str = NULL;

	str = konf_buf_string(this->buf + this->rpos, this->pos - this->rpos);
	if (str)
		this->rpos += (strlen(str) + 1);

	return str;
}

/*--------------------------------------------------------- */
int konf_buf_lseek(konf_buf_t *this, int newpos)
{
	if (newpos > this->pos)
		return -1;
	this->rpos = newpos;

	return newpos;
}

/*--------------------------------------------------------- */
int konf_buf__get_fd(const konf_buf_t * this)
{
	return this->fd;
}

/*--------------------------------------------------------- */
int konf_buf__get_len(const konf_buf_t *this)
{
	return this->pos;
}

/*--------------------------------------------------------- */
char * konf_buf__dup_line(const konf_buf_t *this)
{
	char *str;

	str = malloc(this->pos + 1);
	memcpy(str, this->buf, this->pos);
	str[this->pos] = '\0';
	return str;
}

/*--------------------------------------------------------- */
char * konf_buf__get_buf(const konf_buf_t *this)
{
	return this->buf;
}

/*--------------------------------------------------------- */
void * konf_buf__get_data(const konf_buf_t *this)
{
	return this->data;
}

/*--------------------------------------------------------- */
void konf_buf__set_data(konf_buf_t *this, void *data)
{
	this->data = data;
}

/*---------------------------------------------------------
 * buftree functions
 *--------------------------------------------------------- */

/*--------------------------------------------------------- */
static int find_fd(const void *key, const void *data)
{
	const int *fd = (const int *)key;
	const konf_buf_t *d = (const konf_buf_t *)data;
	return (*fd - d->fd);
}

/*--------------------------------------------------------- */
konf_buf_t *konf_buftree_find(lub_list_t *this, int fd)
{
	return lub_list_find(this, find_fd, &fd);
}

/*--------------------------------------------------------- */
void konf_buftree_remove(lub_list_t *this, int fd)
{
	lub_list_node_t *t;

	if ((t = lub_list_find_node(this, find_fd, &fd)) == NULL)
		return;

	konf_buf_delete((konf_buf_t *)lub_list_node__get_data(t));
	lub_list_del(this, t);
}
