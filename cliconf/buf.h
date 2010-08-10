/*
 * buf.h
 */
 /**
\ingroup clish
\defgroup clish_conf config
@{

\brief This class is a config in memory container.

Use it to implement config in memory.

*/
#ifndef _cliconf_buf_h
#define _cliconf_buf_h

#include <stdio.h>

#include "cliconf/buf/private.h"
#include "lub/bintree.h"

typedef struct conf_buf_s conf_buf_t;

/*=====================================
 * CONF INTERFACE
 *===================================== */
/*-----------------
 * meta functions
 *----------------- */
conf_buf_t *conf_buf_new(int sock);
int conf_buf_bt_compare(const void *clientnode, const void *clientkey);
void conf_buf_bt_getkey(const void *clientnode, lub_bintree_key_t * key);
size_t conf_buf_bt_offset(void);
/*-----------------
 * methods
 *----------------- */
void conf_buf_delete(conf_buf_t *instance);
int conf_buf_read(conf_buf_t *instance);
int conf_buf_add(conf_buf_t *instance, void *str, size_t len);
char * conf_buf_string(char *instance, int len);
char * conf_buf_parse(conf_buf_t *instance);
char * conf_buf_preparse(conf_buf_t *instance);
int conf_buf_lseek(conf_buf_t *instance, int newpos);
int conf_buf__get_sock(const conf_buf_t *instance);
int conf_buf__get_len(const conf_buf_t *instance);

int conf_buftree_read(lub_bintree_t *instance, int sock);
char * conf_buftree_parse(lub_bintree_t *instance, int sock);
void conf_buftree_remove(lub_bintree_t *instance, int sock);


#endif				/* _cliconf_h */
/** @} clish_conf */
