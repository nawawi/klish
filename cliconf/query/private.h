#ifndef query_private_h
#define query_private_h

#include "cliconf/query.h"
#include "lub/types.h"

struct query_s {
	query_op_t op;
	char *pattern;
	unsigned short priority;
	unsigned pwdc;
	char **pwd;
	char *line;
	char *path;
	bool_t splitter;
};

#endif
