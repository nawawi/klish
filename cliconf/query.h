#ifndef _query_h
#define _query_h


typedef enum
{
  QUERY_OP_NONE,
  QUERY_OP_OK,
  QUERY_OP_ERROR,
  QUERY_OP_SET,
  QUERY_OP_UNSET,
  QUERY_OP_STREAM,
  QUERY_OP_DUMP
} query_op_t;

typedef struct query_s query_t;

#include "cliconf/query/private.h"

int query_parse(query_t *query, int argc, char **argv);
int query_parse_str(query_t *query, char *str);
query_t *query_new(void);
void query_free(query_t *query);
char *query__get_pwd(query_t *query, unsigned index);
int query__get_pwdc(query_t *query);
void query_dump(query_t *query);
query_op_t query__get_op(query_t *query);
char * query__get_path(query_t *query);
const char *query__get_pattern(query_t *instance);

#endif
