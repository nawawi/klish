#ifndef _konf_query_private_h
#define _konf_query_private_h

#include "konf/query.h"
#include "lub/types.h"

struct konf_query_s {
	konf_query_op_t op;
	char *pattern;
	unsigned short priority;
	unsigned pwdc;
	char **pwd;
	char *line;
	char *path;
	bool_t splitter;
};

#endif
