/*
 * ptype.h
 */
#include "clish/pargv.h"
#include "lub/argv.h"

#include <sys/types.h>
#include <regex.h>

typedef struct clish_ptype_integer_s clish_ptype_integer_t;
struct clish_ptype_integer_s {
	int min;
	int max;
};

typedef struct clish_ptype_select_s clish_ptype_select_t;
struct clish_ptype_select_s {
	lub_argv_t *items;
};

typedef struct clish_ptype_regex_s clish_ptype_regex_t;
struct clish_ptype_regex_s {
	bool_t is_compiled;
	regex_t re;
};

struct clish_ptype_s {
	char *name;
	char *text;
	char *pattern;
	char *range;
	clish_ptype_method_e method;
	clish_ptype_preprocess_e preprocess;
	unsigned int last_name; /* Index used for auto-completion */
	union {
		clish_ptype_regex_t regex;
		clish_ptype_integer_t integer;
		clish_ptype_select_t select;
	} u;
	clish_action_t *action;
};
