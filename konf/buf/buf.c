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

/*---------------------------------------------------------
 * PRIVATE META FUNCTIONS
 *--------------------------------------------------------- */
int konf_buf_bt_compare(const void *clientnode, const void *clientkey)
{
	const konf_buf_t *this = clientnode;
	int keysock;

	memcpy(&keysock, clientkey, sizeof(keysock));

	return (this->sock - keysock);
}

/*-------------------------------------------------------- */
static void konf_buf_key(lub_bintree_key_t * key,
	int sock)
{
	memcpy(key, &sock, sizeof(sock));
}

/*-------------------------------------------------------- */
void konf_buf_bt_getkey(const void *clientnode, lub_bintree_key_t * key)
{
	const konf_buf_t *this = clientnode;

	konf_buf_key(key, this->sock);
}

/*---------------------------------------------------------
 * PRIVATE METHODS
 *--------------------------------------------------------- */
static void
konf_buf_init(konf_buf_t * this, int sock)
{
	this->sock = sock;
	this->buf = malloc(KONF_BUF_CHUNK);
	this->size = KONF_BUF_CHUNK;
	this->pos = 0;
	this->rpos = 0;

	/* Be a good binary tree citizen */
	lub_bintree_node_init(&this->bt_node);
}

/*--------------------------------------------------------- */
static void konf_buf_fini(konf_buf_t * this)
{
	free(this->buf);
}

/*---------------------------------------------------------
 * PUBLIC META FUNCTIONS
 *--------------------------------------------------------- */
size_t konf_buf_bt_offset(void)
{
	return offsetof(konf_buf_t, bt_node);
}

/*--------------------------------------------------------- */
konf_buf_t *konf_buf_new(int sock)
{
	konf_buf_t *this = malloc(sizeof(konf_buf_t));

	if (this) {
		konf_buf_init(this, sock);
	}

	return this;
}

/*---------------------------------------------------------
 * PUBLIC METHODS
 *--------------------------------------------------------- */
void konf_buf_delete(konf_buf_t * this)
{
	konf_buf_fini(this);
	free(this);
}

/*--------------------------------------------------------- */
konf_buf_t *konf_buftree_find(lub_bintree_t * this,
	int sock)
{
	lub_bintree_key_t key;

	konf_buf_key(&key, sock);

	return lub_bintree_find(this, &key);
}

/*--------------------------------------------------------- */
void konf_buftree_remove(lub_bintree_t * this,
	int sock)
{
	konf_buf_t *tbuf;

	if ((tbuf = konf_buftree_find(this, sock)) == NULL)
		return;

	lub_bintree_remove(this, tbuf);
	konf_buf_delete(tbuf);
}


/*--------------------------------------------------------- */
static int konf_buf_realloc(konf_buf_t *buf, int addsize)
{
	int chunk = KONF_BUF_CHUNK;
	char *tmpbuf;

	if (addsize > chunk)
		chunk = addsize;
	if ((buf->size - buf->pos) < chunk) {
		tmpbuf = realloc(buf->buf, buf->size + chunk);
		buf->buf = tmpbuf;
		buf->size += chunk;
	}

	return buf->size;
}

/*--------------------------------------------------------- */
int konf_buf_add(konf_buf_t *buf, void *str, size_t len)
{
	char *buffer;

	konf_buf_realloc(buf, len);
	buffer = buf->buf + buf->pos;
	memcpy(buffer, str, len);
	buf->pos += len;

	return len;
}

/*--------------------------------------------------------- */
int konf_buf_read(konf_buf_t *buf)
{
	char *buffer;
	int buffer_size;
	int nbytes;

	konf_buf_realloc(buf, 0);
	buffer_size = buf->size - buf->pos;
	buffer = buf->buf + buf->pos;

	nbytes = read(buf->sock, buffer, buffer_size);
	if (nbytes > 0)
		buf->pos += nbytes;

	return nbytes;
}

/*--------------------------------------------------------- */
int konf_buftree_read(lub_bintree_t * this,
	int sock)
{
	konf_buf_t *buf;

	buf = konf_buftree_find(this, sock);
	if (!buf)
		return -1;

	return konf_buf_read(buf);
}

/*--------------------------------------------------------- */
char * konf_buf_string(char *buf, int len)
{
	unsigned i;
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
char * konf_buf_parse(konf_buf_t *buf)
{
	char * str = NULL;

	/* Search the buffer for the string */
	str = konf_buf_string(buf->buf, buf->pos);

	/* Remove parsed string from the buffer */
	if (str) {
		int len = strlen(str) + 1;
		memmove(buf->buf, &buf->buf[len], buf->pos - len);
		buf->pos -= len;
		if (buf->rpos >= len)
			buf->rpos -= len;
		else
			buf->rpos = 0;
	}

	/* Make buffer shorter */
	if ((buf->size - buf->pos) > (2 * KONF_BUF_CHUNK)) {
		char *tmpbuf;
		tmpbuf = realloc(buf->buf, buf->size - KONF_BUF_CHUNK);
		buf->buf = tmpbuf;
		buf->size -= KONF_BUF_CHUNK;
	}

	return str;
}

char * konf_buf_preparse(konf_buf_t *buf)
{
	char * str = NULL;

	str = konf_buf_string(buf->buf + buf->rpos, buf->pos - buf->rpos);
	if (str)
		buf->rpos += (strlen(str) + 1);

	return str;
}

int konf_buf_lseek(konf_buf_t *buf, int newpos)
{
	if (newpos > buf->pos)
		return -1;
	buf->rpos = newpos;

	return newpos;
}

/*--------------------------------------------------------- */
char * konf_buftree_parse(lub_bintree_t * this,
	int sock)
{
	konf_buf_t *buf;

	buf = konf_buftree_find(this, sock);
	if (!buf)
		return NULL;

	return konf_buf_parse(buf);
}

/*--------------------------------------------------------- */
int konf_buf__get_sock(const konf_buf_t * this)
{
	return this->sock;
}

int konf_buf__get_len(const konf_buf_t *this)
{
	return this->pos;
}

