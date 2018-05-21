/*
 * parg.c
 */
#include "private.h"
#include "lub/string.h"
#include "lub/argv.h"
#include "lub/system.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

CLISH_GET_STR(parg, value);

/*--------------------------------------------------------- */
_CLISH_GET_STR(parg, name)
{
	if (!inst)
		return NULL;
	return clish_param__get_name(inst->param);
}

/*--------------------------------------------------------- */
_CLISH_GET(parg, clish_ptype_t *, ptype)
{
	if (!inst)
		return NULL;
	return clish_param__get_ptype(inst->param);
}
