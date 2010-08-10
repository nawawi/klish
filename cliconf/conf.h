/*
 * conf.h
 */
 /**
\ingroup clish
\defgroup clish_conf config
@{

\brief This class is a config in memory container.

Use it to implement config in memory.

*/
#ifndef _cliconf_h
#define _cliconf_h

#include <stdio.h>

#include "cliconf/conf/private.h"

#include "lub/types.h"
#include "lub/bintree.h"

typedef struct cliconf_s cliconf_t;

/*=====================================
 * CONF INTERFACE
 *===================================== */
/*-----------------
 * meta functions
 *----------------- */
cliconf_t *cliconf_new(const char * line, unsigned short priority);
int cliconf_bt_compare(const void *clientnode, const void *clientkey);
void cliconf_bt_getkey(const void *clientnode, lub_bintree_key_t * key);
size_t cliconf_bt_offset(void);

/*-----------------
 * methods
 *----------------- */
void cliconf_delete(cliconf_t * instance);
void cliconf_fprintf(cliconf_t * instance, FILE * stream,
		const char *pattern,
		int depth, unsigned char prev_pri_hi);
cliconf_t *cliconf_new_conf(cliconf_t * instance,
				const char *line, unsigned short priority);
cliconf_t *cliconf_find_conf(cliconf_t * instance,
				const char *line, unsigned short priority);
void cliconf_del_pattern(cliconf_t *this,
				const char *pattern);

/*-----------------
 * attributes 
 *----------------- */
unsigned cliconf__get_depth(const cliconf_t * instance);
unsigned short cliconf__get_priority(const cliconf_t * instance);
unsigned char cliconf__get_priority_hi(const cliconf_t * instance);
unsigned char cliconf__get_priority_lo(const cliconf_t * instance);
bool_t cliconf__get_splitter(const cliconf_t * instance);
void cliconf__set_splitter(cliconf_t *instance, bool_t splitter);

#endif				/* _cliconf_h */
/** @} clish_conf */
