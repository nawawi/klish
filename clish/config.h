/*
 * config.h
 */
#ifndef _clish_config_h
#define _clish_config_h

#include "clish/macros.h"
#include "lub/types.h"

typedef struct clish_config_s clish_config_t;

/* Possible CONFIG operations */
typedef enum {
	CLISH_CONFIG_NONE,
	CLISH_CONFIG_SET,
	CLISH_CONFIG_UNSET,
	CLISH_CONFIG_DUMP
} clish_config_op_e;

clish_config_t *clish_config_new(void);

void clish_config_delete(clish_config_t *instance);
void clish_config_dump(const clish_config_t *instance);

_CLISH_SET(config, clish_config_op_e, op);
_CLISH_GET(config, clish_config_op_e, op);
_CLISH_SET(config, unsigned short, priority);
_CLISH_GET(config, unsigned short, priority);
_CLISH_SET(config, bool_t, splitter);
_CLISH_GET(config, bool_t, splitter);
_CLISH_SET(config, bool_t, unique);
_CLISH_GET(config, bool_t, unique);
_CLISH_SET_STR_ONCE(config, pattern);
_CLISH_GET_STR(config, pattern);
_CLISH_SET_STR_ONCE(config, file);
_CLISH_GET_STR(config, file);
_CLISH_SET_STR_ONCE(config, seq);
_CLISH_GET_STR(config, seq);
_CLISH_SET_STR_ONCE(config, depth);
_CLISH_GET_STR(config, depth);

#endif				/* _clish_config_h */
