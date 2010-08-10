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

#define CONF_BUF_CHUNK 1024

/*---------------------------------------------------------
 * PRIVATE META FUNCTIONS
 *--------------------------------------------------------- */
int conf_buf_bt_compare(const void *clientnode, const void *clientkey)
{
	const conf_buf_t *this = clientnode;
	int keysock;

	memcpy(&keysock, clientkey, sizeof(keysock));

	return (this->sock - keysock);
}

/*-------------------------------------------------------- */
static void conf_buf_key(lub_bintree_key_t * key,
	int sock)
{
	memcpy(key, &sock, sizeof(sock));
}

/*-------------------------------------------------------- */
void conf_buf_bt_getkey(const void *clientnode, lub_bintree_key_t * key)
{
	const conf_buf_t *this = clientnode;

	conf_buf_key(key, this->sock);
}

/*---------------------------------------------------------
 * PRIVATE METHODS
 *--------------------------------------------------------- */
static void
conf_buf_init(conf_buf_t * this, int sock)
{
	this->sock = sock;
	this->buf = malloc(CONF_BUF_CHUNK);
	this->size = CONF_BUF_CHUNK;
	this->pos = 0;
	this->rpos = 0;

	/* Be a good binary tree citizen */
	lub_bintree_node_init(&this->bt_node);
}

/*--------------------------------------------------------- */
static void conf_buf_fini(conf_buf_t * this)
{
	free(this->buf);
}

/*---------------------------------------------------------
 * PUBLIC META FUNCTIONS
 *--------------------------------------------------------- */
size_t conf_buf_bt_offset(void)
{
	return offsetof(conf_buf_t, bt_node);
}

/*--------------------------------------------------------- */
conf_buf_t *conf_buf_new(int sock)
{
	conf_buf_t *this = malloc(sizeof(conf_buf_t));

	if (this) {
		conf_buf_init(this, sock);
	}

	return this;
}

/*---------------------------------------------------------
 * PUBLIC METHODS
 *--------------------------------------------------------- */
void conf_buf_delete(conf_buf_t * this)
{
	conf_buf_fini(this);
	free(this);
}

/*--------------------------------------------------------- */
conf_buf_t *conf_buftree_find(lub_bintree_t * this,
	int sock)
{
	lub_bintree_key_t key;

	conf_buf_key(&key, sock);

	return lub_bintree_find(this, &key);
}

/*--------------------------------------------------------- */
void conf_buftree_remove(lub_bintree_t * this,
	int sock)
{
	conf_buf_t *tbuf;

	if ((tbuf = conf_buftree_find(this, sock)) == NULL)
		return;

	lub_bintree_remove(this, tbuf);
	conf_buf_delete(tbuf);
}


/*--------------------------------------------------------- */
static int conf_buf_realloc(conf_buf_t *buf, int addsize)
{
	int chunk = CONF_BUF_CHUNK;
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
int conf_buf_add(conf_buf_t *buf, void *str, size_t len)
{
	char *buffer;

	conf_buf_realloc(buf, len);
	buffer = buf->buf + buf->pos;
	memcpy(buffer, str, len);
	buf->pos += len;

	return len;
}

/*--------------------------------------------------------- */
int conf_buf_read(conf_buf_t *buf)
{
	char *buffer;
	int buffer_size;
	int nbytes;

	conf_buf_realloc(buf, 0);
	buffer_size = buf->size - buf->pos;
	buffer = buf->buf + buf->pos;

	nbytes = read(buf->sock, buffer, buffer_size);
	if (nbytes > 0)
		buf->pos += nbytes;

	return nbytes;
}

/*--------------------------------------------------------- */
int conf_buftree_read(lub_bintree_t * this,
	int sock)
{
	conf_buf_t *buf;

	buf = conf_buftree_find(this, sock);
	if (!buf)
		return -1;

	return conf_buf_read(buf);
}

/*--------------------------------------------------------- */
char * conf_buf_string(char *buf, int len)
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
char * conf_buf_parse(conf_buf_t *buf)
{
	char * str = NULL;

	/* Search the buffer for the string */
	str = conf_buf_string(buf->buf, buf->pos);

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
	if ((buf->size - buf->pos) > (2 * CONF_BUF_CHUNK)) {
		char *tmpbuf;
		tmpbuf = realloc(buf->buf, buf->size - CONF_BUF_CHUNK);
		buf->buf = tmpbuf;
		buf->size -= CONF_BUF_CHUNK;
	}

	return str;
}

char * conf_buf_preparse(conf_buf_t *buf)
{
	char * str = NULL;

	str = conf_buf_string(buf->buf + buf->rpos, buf->pos - buf->rpos);
	if (str)
		buf->rpos += (strlen(str) + 1);

	return str;
}

int conf_buf_lseek(conf_buf_t *buf, int newpos)
{
	if (newpos > buf->pos)
		return -1;
	buf->rpos = newpos;

	return newpos;
}

/*--------------------------------------------------------- */
char * conf_buftree_parse(lub_bintree_t * this,
	int sock)
{
	conf_buf_t *buf;

	buf = conf_buftree_find(this, sock);
	if (!buf)
		return NULL;

	return conf_buf_parse(buf);
}

/*--------------------------------------------------------- */
int conf_buf__get_sock(const conf_buf_t * this)
{
	return this->sock;
}

int conf_buf__get_len(const conf_buf_t *this)
{
	return this->pos;
}

