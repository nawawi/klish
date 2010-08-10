/*
 * clish_config_callback.c
 *
 *
 * Callback hook to execute config operations.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <assert.h>

#include "clish/private.h"
#include "cliconf/query.h"
#include "lub/argv.h"
#include "lub/string.h"

query_t *query_new(void)
{
	query_t *query;

	if (!(query = malloc(sizeof(*query))))
		return NULL;

	query->op = QUERY_OP_NONE;
	query->pattern = NULL;
	query->priority = 0x7f00;
	query->pwdc = 0;
	query->pwd = NULL;
	query->line = NULL;
	query->path = NULL;
	query->splitter = BOOL_TRUE;

	return query;
}

void query_dump(query_t *query)
{
	switch (query->op) {
	case QUERY_OP_SET:
		printf("op=SET\n");
		break;
	case QUERY_OP_UNSET:
		printf("op=UNSET\n");
		break;
	case QUERY_OP_DUMP:
		printf("op=DUMP\n");
		break;
	case QUERY_OP_OK:
		printf("op=OK\n");
		break;
	case QUERY_OP_ERROR:
		printf("op=ERROR\n");
		break;
	case QUERY_OP_STREAM:
		printf("op=STREAM\n");
		break;
	default:
		printf("op=UNKNOWN\n");
		break;
	}
	printf("pattern=%s\n", query->pattern);
	printf("priority=%u\n", query->priority);
	printf("line=%s\n", query->line);
	printf("path=%s\n", query->path);
	printf("pwdc=%u\n", query->pwdc);
}

void query_add_pwd(query_t *query, char *str)
{
	size_t new_size;
	char **tmp;

	if (!query)
		return;

	new_size = ((query->pwdc + 1) * sizeof(char *));

	/* resize the pwd vector */
	tmp = realloc(query->pwd, new_size);
	assert(tmp);
	query->pwd = tmp;
	/* insert reference to the pwd component */
	query->pwd[query->pwdc++] = lub_string_dup(str);
}

char * query__get_pwd(query_t *query, unsigned index)
{
	if (!query)
		return NULL;
	if (index >= query->pwdc)
		return NULL;

	return query->pwd[index];
}

int query__get_pwdc(query_t *query)
{
	return query->pwdc;
}

query_op_t query__get_op(query_t *query)
{
	return query->op;
}

char * query__get_path(query_t *query)
{
	return query->path;
}

void query_free(query_t *query)
{
	unsigned i;

	lub_string_free(query->pattern);
	lub_string_free(query->line);
	lub_string_free(query->path);
	if (query->pwdc > 0) {
		for (i = 0; i < query->pwdc; i++)
			lub_string_free(query->pwd[i]);
		free(query->pwd);
	}

	free(query);
}

/* Parse query */
int query_parse(query_t *query, int argc, char **argv)
{
	unsigned i = 0;
	int pwdc = 0;

	static const char *shortopts = "suoedtp:r:l:f:i";
/*	static const struct option longopts[] = {
		{"set",		0, NULL, 's'},
		{"unset",	0, NULL, 'u'},
		{"ok",		0, NULL, 'o'},
		{"error",	0, NULL, 'e'},
		{"dump",	0, NULL, 'd'},
		{"stream",	0, NULL, 't'},
		{"priority",	1, NULL, 'p'},
		{"pattern",	1, NULL, 'r'},
		{"line",	1, NULL, 'l'},
		{"file",	1, NULL, 'f'},
		{"splitter",	0, NULL, 'i'},
		{NULL,		0, NULL, 0}
	};
*/

	optind = 0;
	while(1) {
		int opt;
/*		opt = getopt_long(argc, argv, shortopts, longopts, NULL); */
		opt = getopt(argc, argv, shortopts);
		if (-1 == opt)
			break;
		switch (opt) {
		case 'o':
			query->op = QUERY_OP_OK;
			break;
		case 'e':
			query->op = QUERY_OP_ERROR;
			break;
		case 's':
			query->op = QUERY_OP_SET;
			break;
		case 'u':
			query->op = QUERY_OP_UNSET;
			break;
		case 'd':
			query->op = QUERY_OP_DUMP;
			break;
		case 't':
			query->op = QUERY_OP_STREAM;
			break;
		case 'p':
			{
			long val = 0;
			char *endptr;
			unsigned short pri;

			val = strtol(optarg, &endptr, 0);
			if (endptr == optarg)
				break;
			if ((val > 0xffff) || (val < 0))
				break;
			pri = (unsigned short)val;
			query->priority = pri;
			break;
			}
		case 'r':
			query->pattern = lub_string_dup(optarg);
			break;
		case 'l':
			query->line = lub_string_dup(optarg);
			break;
		case 'f':
			query->path = lub_string_dup(optarg);
			break;
		case 'i':
			query->splitter = BOOL_FALSE;
			break;
		default:
			break;
		}
	}

	/* Check options */
	if (QUERY_OP_NONE == query->op)
		return -1;
	if (QUERY_OP_SET == query->op) {
		if (NULL == query->pattern)
			return -1;
		if (NULL == query->line)
			return -1;
	}

	if ((pwdc = argc - optind) < 0)
		return -1;

	for (i = 0; i < pwdc; i ++)
		query_add_pwd(query, argv[optind + i]);

	return 0;
}

/* Parse query string */
int query_parse_str(query_t *query, char *str)
{
	int res;
	lub_argv_t *lub_argv;
	char **str_argv;
	int str_argc;

	/* Make args from string */
	lub_argv = lub_argv_new(str, 0);
	str_argv = lub_argv__get_argv(lub_argv, "");
	str_argc = lub_argv__get_count(lub_argv) + 1;

	/* Parse query */
	res = query_parse(query, str_argc, str_argv);
	free(str_argv);
	lub_argv_delete(lub_argv);

	return res;
}

const char * query__get_pattern(query_t *this)
{
	return this->pattern;
}
