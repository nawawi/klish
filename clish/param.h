/*
 * param.h
 * Parameter instances are assocated with a command line and used to validate the
 * the arguments which a user is inputing for a command.
 */

#ifndef _clish_param_h
#define _clish_param_h

typedef struct clish_paramv_s clish_paramv_t;
typedef struct clish_param_s clish_param_t;

#include "clish/types.h"
#include "clish/ptype.h"
#include "clish/pargv.h"
#include "clish/var.h"

/* The means by which the param is interpreted */
typedef enum {
	/* A common parameter */
	CLISH_PARAM_COMMON,
	/* A swich parameter. Choose the only one of nested parameters. */
	CLISH_PARAM_SWITCH,
	/* A subcomand. Identified by it's name. */
	CLISH_PARAM_SUBCOMMAND
} clish_param_mode_e;

/* Class param */

clish_param_t *clish_param_new(const char *name,
	const char *text, const char *ptype_name);

void clish_param_delete(clish_param_t * instance);
void clish_param_help(const clish_param_t * instance, clish_help_t *help);
void clish_param_help_arrow(const clish_param_t * instance, size_t offset);
char *clish_param_validate(const clish_param_t * instance, const char *text);
void clish_param_dump(const clish_param_t * instance);
void clish_param_insert_param(clish_param_t * instance, clish_param_t * param);

_CLISH_SET_STR(param, ptype_name);
_CLISH_GET_STR(param, ptype_name);
_CLISH_SET_STR(param, access);
_CLISH_GET_STR(param, access);
_CLISH_GET_STR(param, name);
_CLISH_GET_STR(param, text);
_CLISH_GET_STR(param, range);
_CLISH_SET_STR_ONCE(param, value);
_CLISH_GET_STR(param, value);
_CLISH_SET(param, clish_ptype_t *, ptype);
_CLISH_GET(param, clish_ptype_t *, ptype);
_CLISH_SET_STR_ONCE(param, defval);
_CLISH_GET_STR(param, defval);
_CLISH_SET_STR_ONCE(param, test);
_CLISH_GET_STR(param, test);
_CLISH_SET_STR_ONCE(param, completion);
_CLISH_GET_STR(param, completion);
_CLISH_SET(param, clish_param_mode_e, mode);
_CLISH_GET(param, clish_param_mode_e, mode);
_CLISH_GET(param, clish_paramv_t *, paramv);
_CLISH_SET(param, bool_t, optional);
_CLISH_GET(param, bool_t, optional);
_CLISH_SET(param, bool_t, order);
_CLISH_GET(param, bool_t, order);
_CLISH_SET(param, bool_t, hidden);
_CLISH_GET(param, bool_t, hidden);
_CLISH_GET(param, unsigned int, param_count);

clish_param_t *clish_param__get_param(const clish_param_t * instance,
	unsigned int index);

/* Class paramv */

clish_paramv_t *clish_paramv_new(void);
void clish_paramv_delete(clish_paramv_t * instance);
void clish_paramv_insert(clish_paramv_t * instance, clish_param_t * param);
int clish_paramv_remove(clish_paramv_t *instance, unsigned int index); /* Remove param from vector */
clish_param_t *clish_paramv__get_param(const clish_paramv_t * instance,
				unsigned index);
unsigned int clish_paramv__get_count(const clish_paramv_t * instance);
clish_param_t *clish_paramv_find_param(const clish_paramv_t * instance,
	const char *name);
const char *clish_paramv_find_default(const clish_paramv_t * instance,
	const char *name);

#endif				/* _clish_param_h */
