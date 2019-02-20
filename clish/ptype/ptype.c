/*
 * ptype.c
 */
#include "private.h"
#include "lub/string.h"
#include "lub/ctype.h"
#include "lub/argv.h"
#include "lub/conv.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <stdio.h>

/*--------------------------------------------------------- */
int clish_ptype_compare(const void *first, const void *second)
{
	const clish_ptype_t *f = (const clish_ptype_t *)first;
	const clish_ptype_t *s = (const clish_ptype_t *)second;

	return strcmp(f->name, s->name);
}

/*--------------------------------------------------------- */
static void clish_ptype_init(clish_ptype_t * this,
	const char *name, const char *text, const char *pattern,
	clish_ptype_method_e method, clish_ptype_preprocess_e preprocess)
{
	assert(this);
	assert(name);
	this->name = lub_string_dup(name);
	this->text = NULL;
	this->pattern = NULL;
	this->preprocess = preprocess;
	this->range = NULL;
	this->action = clish_action_new();

	if (pattern) {
		/* set the pattern for this type */
		clish_ptype__set_pattern(this, pattern, method);
	} else {
		/* The method is regexp by default */
		this->method = CLISH_PTYPE_METHOD_REGEXP;
	}
	
	/* set the help text for this type */
	if (text)
		clish_ptype__set_text(this, text);
}

/*--------------------------------------------------------- */
static void clish_ptype_fini(clish_ptype_t * this)
{
	if (this->pattern) {
		switch (this->method) {
		case CLISH_PTYPE_METHOD_REGEXP:
			if (this->u.regex.is_compiled)
				regfree(&this->u.regex.re);
			break;
		case CLISH_PTYPE_METHOD_INTEGER:
		case CLISH_PTYPE_METHOD_UNSIGNEDINTEGER:
			break;
		case CLISH_PTYPE_METHOD_SELECT:
			lub_argv_delete(this->u.select.items);
			break;
		default:
			break;
		}
	}

	lub_string_free(this->name);
	lub_string_free(this->text);
	lub_string_free(this->pattern);
	lub_string_free(this->range);
	clish_action_delete(this->action);
}

/*--------------------------------------------------------- */
clish_ptype_t *clish_ptype_new(const char *name,
	const char *help, const char *pattern,
	clish_ptype_method_e method, clish_ptype_preprocess_e preprocess)
{
	clish_ptype_t *this = malloc(sizeof(clish_ptype_t));

	if (this)
		clish_ptype_init(this, name, help, pattern, method, preprocess);
	return this;
}

/*--------------------------------------------------------- */
void clish_ptype_free(void *data)
{
	clish_ptype_t *this = (clish_ptype_t *)data;
	clish_ptype_fini(this);
	free(this);
}

/*--------------------------------------------------------- */
static char *clish_ptype_select__get_name(const clish_ptype_t *this,
	unsigned int index)
{
	char *res = NULL;
	size_t name_len;
	const char *arg = lub_argv__get_arg(this->u.select.items, index);

	if (!arg)
		return NULL;
	name_len = strlen(arg);
	const char *lbrk = strchr(arg, '(');
	if (lbrk)
		name_len = (size_t) (lbrk - arg);
	res = lub_string_dupn(arg, name_len);

	return res;
}

/*--------------------------------------------------------- */
static char *clish_ptype_select__get_value(const clish_ptype_t *this,
	unsigned int index)
{
	char *res = NULL;
	const char *lbrk, *rbrk, *value;
	size_t value_len;
	const char *arg = lub_argv__get_arg(this->u.select.items, index);

	if (!arg)
		return NULL;

	lbrk = strchr(arg, '(');
	rbrk = strchr(arg, ')');
	value = arg;
	value_len = strlen(arg);
	if (lbrk) {
		value = lbrk + 1;
		if (rbrk)
			value_len = (size_t) (rbrk - value);
	}
	res = lub_string_dupn(value, value_len);

	return res;
}

/*--------------------------------------------------------- */
static void clish_ptype__set_range(clish_ptype_t * this)
{
	char tmp[80];

	/* Now set up the range values */
	switch (this->method) {
	/*------------------------------------------------- */
	case CLISH_PTYPE_METHOD_REGEXP:
		/* Nothing more to do */
		break;
	/*------------------------------------------------- */
	case CLISH_PTYPE_METHOD_INTEGER:
		/* Setup the integer range */
		snprintf(tmp, sizeof(tmp), "%d..%d",
			this->u.integer.min, this->u.integer.max);
		tmp[sizeof(tmp) - 1] = '\0';
		this->range = lub_string_dup(tmp);
		break;
	/*------------------------------------------------- */
	case CLISH_PTYPE_METHOD_UNSIGNEDINTEGER:
		/* Setup the unsigned integer range */
		snprintf(tmp, sizeof(tmp), "%u..%u",
			(unsigned int)this->u.integer.min,
			(unsigned int)this->u.integer.max);
		tmp[sizeof(tmp) - 1] = '\0';
		this->range = lub_string_dup(tmp);
		break;
	/*------------------------------------------------- */
	case CLISH_PTYPE_METHOD_SELECT:
	{
		/* Setup the selection values to the help text */
		unsigned int i;

		for (i = 0; i < lub_argv__get_count(this->u.select.items); i++) {
			char *name = clish_ptype_select__get_name(this, i);

			if (i > 0)
				lub_string_cat(&this->range, "/");
			snprintf(tmp, sizeof(tmp), "%s", name);
			tmp[sizeof(tmp) - 1] = '\0';
			lub_string_cat(&this->range, tmp);
			lub_string_free(name);
		}
		break;
	}
	/*------------------------------------------------- */
	default:
		break;
	/*------------------------------------------------- */
	}
}

/*--------------------------------------------------------- */
static const char *method_names[] = {
	"regexp",
	"integer",
	"unsignedInteger",
	"select",
	"code"
};

/*--------------------------------------------------------- */
const char *clish_ptype__get_method_name(clish_ptype_method_e method)
{
	if (method >= CLISH_PTYPE_METHOD_MAX)
		return NULL;
	return method_names[method];
}

/*--------------------------------------------------------- */
/* Return value of CLISH_PTYPE_METHOD_MAX indicates an illegal method */
clish_ptype_method_e clish_ptype_method_resolve(const char *name)
{
	unsigned int i;

	if (NULL == name)
		return CLISH_PTYPE_METHOD_REGEXP;
	for (i = 0; i < CLISH_PTYPE_METHOD_MAX; i++) {
		if (!strcmp(name, method_names[i]))
			break;
	}

	return (clish_ptype_method_e)i;
}

/*--------------------------------------------------------- */
static const char *preprocess_names[] = {
	"none",
	"toupper",
	"tolower"
};

/*--------------------------------------------------------- */
const char *clish_ptype__get_preprocess_name(clish_ptype_preprocess_e preprocess)
{
	if (preprocess >= CLISH_PTYPE_PRE_MAX)
		return NULL;

	return preprocess_names[preprocess];
}

/*--------------------------------------------------------- */
/* Return value of CLISH_PTYPE_PRE_MAX indicates an illegal method */
clish_ptype_preprocess_e clish_ptype_preprocess_resolve(const char *name)
{
	unsigned int i;

	if (NULL == name)
		return CLISH_PTYPE_PRE_NONE;
	for (i = 0; i < CLISH_PTYPE_PRE_MAX; i++) {
		if (!strcmp(name, preprocess_names[i]))
			break;
	}

	return (clish_ptype_preprocess_e)i;
}

/*--------------------------------------------------------- */
void clish_ptype_word_generator(clish_ptype_t * this,
	lub_argv_t *matches, const char *text)
{
	char *result = NULL;
	unsigned int i = 0;

	/* Only METHOD_SELECT has completions */
	if (this->method != CLISH_PTYPE_METHOD_SELECT)
		return;

	/* First of all simply try to validate the result */
	result = clish_ptype_validate(this, text);
	if (result) {
		lub_argv_add(matches, result);
		lub_string_free(result);
		return;
	}

	/* Iterate possible completion */
	while ((result = clish_ptype_select__get_name(this, i++))) {
		/* get the next item and check if it is a completion */
		if (result == lub_string_nocasestr(result, text))
			lub_argv_add(matches, result);
		lub_string_free(result);
	}
}

/*--------------------------------------------------------- */
static char *clish_ptype_validate_or_translate(clish_ptype_t * this,
	const char *text, bool_t translate)
{
	char *result = lub_string_dup(text);
	assert(this->pattern);

	switch (this->preprocess) {
	/*----------------------------------------- */
	case CLISH_PTYPE_PRE_NONE:
		break;
	/*----------------------------------------- */
	case CLISH_PTYPE_PRE_TOUPPER:
	{
		char *p = result;
		while (*p) {
			*p = lub_ctype_toupper(*p);
			p++;
		}
		break;
	}
	/*----------------------------------------- */
	case CLISH_PTYPE_PRE_TOLOWER:
	{
		char *p = result;
		while (*p) {
			*p = lub_ctype_tolower(*p);
			p++;
		}
		break;
	}
	/*----------------------------------------- */
	default:
		break;
	}

	/* Validate according the specified method */
	switch (this->method) {
	/*------------------------------------------------- */
	case CLISH_PTYPE_METHOD_REGEXP:
		/* Lazy compilation of the regular expression */
		if (!this->u.regex.is_compiled) {
			if (regcomp(&this->u.regex.re, this->pattern,
				REG_NOSUB | REG_EXTENDED)) {
				lub_string_free(result);
				result = NULL;
				break;
			}
			this->u.regex.is_compiled = BOOL_TRUE;
		}

		if (regexec(&this->u.regex.re, result, 0, NULL, 0)) {
			lub_string_free(result);
			result = NULL;
		}
		break;
	/*------------------------------------------------- */
	case CLISH_PTYPE_METHOD_INTEGER:
	{
		/* first of all check that this is a number */
		bool_t ok = BOOL_TRUE;
		const char *p = result;
		int value = 0;

		if (*p == '-')
			p++;
		while (*p) {
			if (!lub_ctype_isdigit(*p++)) {
				ok = BOOL_FALSE;
				break;
			}
		}
		if (BOOL_FALSE == ok) {
			lub_string_free(result);
			result = NULL;
			break;
		}
		/* Convert to number and check the range */
		if ((lub_conv_atoi(result, &value, 0) < 0) ||
			(value < this->u.integer.min) ||
			(value > this->u.integer.max)) {
			lub_string_free(result);
			result = NULL;
			break;
		}
		break;
	}
	/*------------------------------------------------- */
	case CLISH_PTYPE_METHOD_UNSIGNEDINTEGER:
	{
		/* first of all check that this is a number */
		bool_t ok = BOOL_TRUE;
		const char *p = result;
		unsigned int value = 0;
		while (p && *p) {
			if (!lub_ctype_isdigit(*p++)) {
				ok = BOOL_FALSE;
				break;
			}
		}
		if (BOOL_FALSE == ok) {
			lub_string_free(result);
			result = NULL;
			break;
		}
		/* Convert to number and check the range */
		if ((lub_conv_atoui(result, &value, 0) < 0) ||
			(value < (unsigned)this->u.integer.min) ||
			(value > (unsigned)this->u.integer.max)) {
			lub_string_free(result);
			result = NULL;
			break;
		}
		break;
	}
	/*------------------------------------------------- */
	case CLISH_PTYPE_METHOD_SELECT:
	{
		unsigned int i;
		for (i = 0; i < lub_argv__get_count(this->u.select.items); i++) {
			char *name = clish_ptype_select__get_name(this, i);
			char *value = clish_ptype_select__get_value(this, i);
			int tmp = lub_string_nocasecmp(result, name);
			lub_string_free((BOOL_TRUE == translate) ? name : value);
			if (0 == tmp) {
				lub_string_free(result);
				result = ((BOOL_TRUE == translate) ? value : name);
				break;
			}
			lub_string_free((BOOL_TRUE == translate) ? value : name);
		}
		if (i == lub_argv__get_count(this->u.select.items)) {
			/* failed to find a match */
			lub_string_free(result);
			result = NULL;
			break;
		}
		break;
	}
	/*------------------------------------------------- */
	default:
		break;
	}
	return (char *)result;
}

/*--------------------------------------------------------- */
char *clish_ptype_validate(clish_ptype_t * this, const char *text)
{
	return clish_ptype_validate_or_translate(this, text, BOOL_FALSE);
}

/*--------------------------------------------------------- */
char *clish_ptype_translate(clish_ptype_t * this, const char *text)
{
	return clish_ptype_validate_or_translate(this, text, BOOL_TRUE);
}

CLISH_GET_STR(ptype, name);
CLISH_SET_STR_ONCE(ptype, text);
CLISH_GET_STR(ptype, text);
CLISH_SET_ONCE(ptype, clish_ptype_preprocess_e, preprocess);
CLISH_GET_STR(ptype, range);
CLISH_GET(ptype, clish_action_t *, action);

/*--------------------------------------------------------- */
void clish_ptype__set_pattern(clish_ptype_t * this,
	const char *pattern, clish_ptype_method_e method)
{
	assert(NULL == this->pattern);
	this->method = method;

	switch (this->method) {
	/*------------------------------------------------- */
	case CLISH_PTYPE_METHOD_REGEXP:
	{
		lub_string_cat(&this->pattern, "^");
		lub_string_cat(&this->pattern, pattern);
		lub_string_cat(&this->pattern, "$");
		/* Use lazy mechanism to compile regular expressions */
		this->u.regex.is_compiled = BOOL_FALSE;
		break;
	}
	/*------------------------------------------------- */
	case CLISH_PTYPE_METHOD_INTEGER:
		/* default the range to that of an integer */
		this->u.integer.min = INT_MIN;
		this->u.integer.max = INT_MAX;
		this->pattern = lub_string_dup(pattern);
		/* now try and read the specified range */
		sscanf(this->pattern, "%d..%d",
			&this->u.integer.min, &this->u.integer.max);
		break;
	/*------------------------------------------------- */
	case CLISH_PTYPE_METHOD_UNSIGNEDINTEGER:
		/* default the range to that of an unsigned integer */
		this->u.integer.min = 0;
		this->u.integer.max = (int)UINT_MAX;
		this->pattern = lub_string_dup(pattern);
		/* now try and read the specified range */
		sscanf(this->pattern, "%u..%u",
			(unsigned int *)&this->u.integer.min,
			(unsigned int *)&this->u.integer.max);
		break;
	/*------------------------------------------------- */
	case CLISH_PTYPE_METHOD_SELECT:
		this->pattern = lub_string_dup(pattern);
		/* store a vector of item descriptors */
		this->u.select.items = lub_argv_new(this->pattern, 0);
		break;
	/*------------------------------------------------- */
	default:
		break;
	}
	/* now set up the range details */
	clish_ptype__set_range(this);
}
