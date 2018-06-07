/*
 * buf.h
 */

#ifndef _konf_buf_h
#define _konf_buf_h

#include <stdio.h>

#include <lub/list.h>

typedef struct konf_buf_s konf_buf_t;

konf_buf_t *konf_buf_new(int fd);
int konf_buf_compare(const void *clientnode, const void *clientkey);
void konf_buf_delete(void *instance);
int konf_buf_read(konf_buf_t *instance);
int konf_buf_add(konf_buf_t *instance, void *str, size_t len);
char * konf_buf_string(char *instance, int len);
char * konf_buf_parse(konf_buf_t *instance);
char * konf_buf_preparse(konf_buf_t *instance);
int konf_buf_lseek(konf_buf_t *instance, int newpos);
int konf_buf__get_fd(const konf_buf_t *instance);
int konf_buf__get_len(const konf_buf_t *instance);
char * konf_buf__dup_line(const konf_buf_t *instance);
char * konf_buf__get_buf(const konf_buf_t *instance);
void * konf_buf__get_data(const konf_buf_t *instance);
void konf_buf__set_data(konf_buf_t *instance, void *data);

void konf_buftree_remove(lub_list_t *instance, int fd);
konf_buf_t *konf_buftree_find(lub_list_t *instance, int fd);

#endif				/* _konf_buf_h */
