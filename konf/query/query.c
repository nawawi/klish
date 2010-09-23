#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <assert.h>

#include "clish/private.h"
#include "private.h"
#include "lub/types.h"
#include "lub/argv.h"
#include "lub/string.h"

konf_query_t *konf_query_new(void)
{
	konf_query_t *query;

	if (!(query = malloc(sizeof(*query))))
		return NULL;

	query->op = KONF_QUERY_OP_NONE;
	query->pattern = NULL;
	query->priority = 0;
	query->seq = BOOL_FALSE;
	query->seq_num = 0;
	query->pwdc = 0;
	query->pwd = NULL;
	query->line = NULL;
	query->path = NULL;
	query->splitter = BOOL_TRUE;

	return query;
}

void konf_query_dump(konf_query_t *query)
{
	switch (query->op) {
	case KONF_QUERY_OP_SET:
		printf("op=SET\n");
		break;
	case KONF_QUERY_OP_UNSET:
		printf("op=UNSET\n");
		break;
	case KONF_QUERY_OP_DUMP:
		printf("op=DUMP\n");
		break;
	case KONF_QUERY_OP_OK:
		printf("op=OK\n");
		break;
	case KONF_QUERY_OP_ERROR:
		printf("op=ERROR\n");
		break;
	case KONF_QUERY_OP_STREAM:
		printf("op=STREAM\n");
		break;
	default:
		printf("op=UNKNOWN\n");
		break;
	}
	printf("pattern=%s\n", query->pattern);
	printf("priority=%u\n", query->priority);
	printf("sequence=%u\n", query->seq);
	printf("seq_num=%u\n", query->seq_num);
	printf("line=%s\n", query->line);
	printf("path=%s\n", query->path);
	printf("pwdc=%u\n", query->pwdc);
}

void konf_query_add_pwd(konf_query_t *query, char *str)
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


void konf_query_free(konf_query_t *query)
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
int konf_query_parse(konf_query_t *query, int argc, char **argv)
{
	unsigned i = 0;
	int pwdc = 0;

	static const char *shortopts = "suoedtp:q:r:l:f:i";
/*	static const struct option longopts[] = {
		{"set",		0, NULL, 's'},
		{"unset",	0, NULL, 'u'},
		{"ok",		0, NULL, 'o'},
		{"error",	0, NULL, 'e'},
		{"dump",	0, NULL, 'd'},
		{"stream",	0, NULL, 't'},
		{"priority",	1, NULL, 'p'},
		{"seq",		1, NULL, 'q'},
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
			query->op = KONF_QUERY_OP_OK;
			break;
		case 'e':
			query->op = KONF_QUERY_OP_ERROR;
			break;
		case 's':
			query->op = KONF_QUERY_OP_SET;
			break;
		case 'u':
			query->op = KONF_QUERY_OP_UNSET;
			break;
		case 'd':
			query->op = KONF_QUERY_OP_DUMP;
			break;
		case 't':
			query->op = KONF_QUERY_OP_STREAM;
			break;
		case 'p':
			{
			long val = 0;
			char *endptr;

			val = strtol(optarg, &endptr, 0);
			if (endptr == optarg)
				break;
			if ((val > 0xffff) || (val < 0))
				break;
			query->priority = (unsigned short)val;
			break;
			}
		case 'q':
			{
			long val = 0;
			char *endptr;

			query->seq = BOOL_TRUE;
			val = strtol(optarg, &endptr, 0);
			if (endptr == optarg)
				break;
			if ((val > 0xffff) || (val < 0))
				break;
			query->seq_num = (unsigned short)val;
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
	if (KONF_QUERY_OP_NONE == query->op)
		return -1;
	if (KONF_QUERY_OP_SET == query->op) {
		if (NULL == query->pattern)
			return -1;
		if (NULL == query->line)
			return -1;
	}

	if ((pwdc = argc - optind) < 0)
		return -1;

	for (i = 0; i < pwdc; i ++)
		konf_query_add_pwd(query, argv[optind + i]);

	return 0;
}

/* Parse query string */
int konf_query_parse_str(konf_query_t *query, char *str)
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
	res = konf_query_parse(query, str_argc, str_argv);
	free(str_argv);
	lub_argv_delete(lub_argv);

	return res;
}

char * konf_query__get_pwd(konf_query_t *query, unsigned index)
{
	if (!query)
		return NULL;
	if (index >= query->pwdc)
		return NULL;

	return query->pwd[index];
}

int konf_query__get_pwdc(konf_query_t *query)
{
	return query->pwdc;
}

konf_query_op_t konf_query__get_op(konf_query_t *query)
{
	return query->op;
}

char * konf_query__get_path(konf_query_t *query)
{
	return query->path;
}


const char * konf_query__get_pattern(konf_query_t *this)
{
	return this->pattern;
}

const char * konf_query__get_line(konf_query_t *this)
{
	return this->line;
}

unsigned short konf_query__get_priority(konf_query_t *this)
{
	return this->priority;
}

bool_t konf_query__get_splitter(konf_query_t *this)
{
	return this->splitter;
}

bool_t konf_query__get_seq(konf_query_t *this)
{
	return this->seq;
}

unsigned short konf_query__get_seq_num(konf_query_t *this)
{
	return this->seq_num;
}
