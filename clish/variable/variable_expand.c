/*
 * shell_variable_expand.c
 */
#include "private.h"
#include "lub/string.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <sys/types.h>
#include <regex.h>

/*----------------------------------------------------------- */
/*
 * search the current viewid string for a variable
 */
static char *find_viewid_var(const char *viewid, const char *name)
{
	char *result = NULL;
	regex_t regex;
	int status;
	char *pattern = NULL;
	regmatch_t pmatches[2];

	/* build up the pattern to match */
	lub_string_cat(&pattern, name);
	lub_string_cat(&pattern, "[ ]*=([^;]*)");

	/* compile the regular expression to find this variable */
	status = regcomp(&regex, pattern, REG_EXTENDED);
	assert(0 == status);
	lub_string_free(pattern);

	/* now perform the matching */
	/*lint -e64 Type mismatch (arg. no. 4) */
	/*
	 * lint seems to equate regmatch_t[] as being of type regmatch_t !!!
	 */
	status = regexec(&regex, viewid, 2, pmatches, 0);
	/*lint +e64 */
	if (0 == status) {
		regoff_t len = pmatches[1].rm_eo - pmatches[1].rm_so;
		const char *value = &viewid[pmatches[1].rm_so];
		/* found a match */
		result = lub_string_dupn(value, (unsigned)len);
	}
	/* release the regular expression */
	regfree(&regex);

	return result;
}

/*----------------------------------------------------------- */
/*
 * expand context dependent fixed-name variables
 */
static char *find_context_var(const context_t * this, const char *name)
{
	char *result = NULL;

	if (!this->cmd)
		return NULL;
	if (!lub_string_nocasecmp(name, "__full_cmd")) {
		result = lub_string_dup(clish_command__get_name(this->cmd));
	} else if (!lub_string_nocasecmp(name, "__cmd")) {
		result = lub_string_dup(clish_command__get_name(
			clish_command__get_cmd(this->cmd)));
	} else if (!lub_string_nocasecmp(name, "__orig_cmd")) {
		result = lub_string_dup(clish_command__get_name(
			clish_command__get_orig(this->cmd)));
	} else if (!lub_string_nocasecmp(name, "__line")) {
		if (this->pargv)
			result = clish_variable__get_line(this->cmd, this->pargv);
	} else if (!lub_string_nocasecmp(name, "__params")) {
		if (this->pargv)
			result = clish_variable__get_params(this->cmd, this->pargv);
	} else if (lub_string_nocasestr(name, "__prefix") == name) {
		int idx = 0;
		int pnum = 0;
		pnum = lub_argv_wordcount(clish_command__get_name(this->cmd)) -
			lub_argv_wordcount(clish_command__get_name(
			clish_command__get_cmd(this->cmd)));
		idx = atoi(name + strlen("__prefix"));
		if (idx < pnum) {
			lub_argv_t *argv = lub_argv_new(
				clish_command__get_name(this->cmd), 0);
			result = lub_string_dup(lub_argv__get_arg(argv, idx));
			lub_argv_delete(argv);
		}
	}

	return result;
}

/*--------------------------------------------------------- */
static char *context_retrieve(const context_t *this, const char *name)
{
	char *result = NULL;
	const char *tmp = NULL;
	const char *escape_chars = NULL;
	char *string = NULL;
	assert(name);

	/* try and substitute a parameter value */
	if (this && this->pargv) {
		const clish_parg_t *parg =
			clish_pargv_find_arg(this->pargv, name);
		/* substitute the command line value */
		if (parg)
			tmp = clish_parg__get_value(parg);
	}
	/* try and substitute the param's default */
	if (!tmp) {
		if (this && this->cmd)
			tmp = clish_paramv_find_default(
				clish_command__get_paramv(this->cmd), name);
	}
	/* try and substitute a viewId variable */
	if (!tmp) {
		if (this && this->viewid)
			tmp = string = find_viewid_var(this->viewid, name);
	}
	/* try and substitute context fixed variable */
	if (!tmp)
		tmp = string = find_context_var(this, name);
	/* get the contents of an environment variable */
	if (!tmp)
		tmp = getenv(name);
	/* override the escape characters */
	if (this && this->cmd)
		escape_chars = clish_command__get_escape_chars(this->cmd);
	result = lub_string_encode(tmp, escape_chars);
	/* free the dynamic memory */
	if (string)
		lub_string_free(string);

	return result;
}

/*--------------------------------------------------------- */
/* 
 * return the next segment of text from the provided string
 * segments are delimited by variables within the string.
 */
static char *context_nextsegment(const context_t * this, const char **string)
{
	const char *p = *string;
	char *result = NULL;
	size_t len = 0;

	if (p) {
		if (*p && (p[0] == '$') && (p[1] == '{')) {
			/* start of a variable */
			const char *tmp;
			p += 2;
			tmp = p;

			/*
			 * find the end of the variable 
			 */
			while (*p && p++[0] != '}') {
				len++;
			}

			/* ignore non-terminated variables */
			if (p[-1] == '}') {
				bool_t valid = BOOL_FALSE;
				char *text, *q;
				char *saveptr;

				/* get the variable text */
				text = lub_string_dupn(tmp, len);
				/*
				 * tokenise this INTO ':' separated words
				 * and either expand or duplicate into the result string.
				 * Only return a result if at least 
				 * of the words is an expandable variable
				 */
				for (q = strtok_r(text, ":", &saveptr);
					q; q = strtok_r(NULL, ":", &saveptr)) {
					char *var = context_retrieve(this, q);

					/* copy the expansion or the raw word */
					lub_string_cat(&result, var ? var : q);

					/* record any expansions */
					if (var)
						valid = BOOL_TRUE;
					lub_string_free(var);
				}

				if (!valid) {
					/* not a valid variable expansion */
					lub_string_free(result);
					result = lub_string_dup("");
				}

				/* finished with the variable text */
				lub_string_free(text);
			}
		} else {
			/* find the start of a variable */
			while (*p) {
				if ((p[0] == '$') && (p[1] == '{'))
					break;
				len++;
				p++;
			}
			if (len > 0)
				result = lub_string_dupn(*string, len);
		}
		/* move the string pointer on for next time... */
		*string = p;
	}
	return result;
}

/*--------------------------------------------------------- */
/*
 * This function builds a dynamic string based on that provided
 * subtituting each occurance of a "${FRED}" type variable sub-string
 * with the appropriate value.
 */
char *clish_variable_expand(const char *string, const char *viewid,
	const clish_command_t * cmd, clish_pargv_t * pargv)
{
	char *seg, *result = NULL;
	context_t context;

	/* setup the context */
	context.viewid = viewid;
	context.cmd = cmd;
	context.pargv = pargv;

	/* read each segment and extend the result */
	while (NULL != (seg = context_nextsegment(&context, &string))) {
		lub_string_cat(&result, seg);
		lub_string_free(seg);
	}

	return result;
}

/*--------------------------------------------------------- */
char *clish_variable__get_params(const clish_command_t * cmd, clish_pargv_t * pargv)
{
	char *line = NULL;
	unsigned i, cnt;
	const clish_param_t *param;
	const clish_parg_t *parg;

	if (!pargv)
		return NULL;

	cnt = clish_pargv__get_count(pargv);
	for (i = 0; i < cnt; i++) {
		const char *tmp;
		char *space = NULL;
		param = clish_pargv__get_param(pargv, i);
		if (clish_param__get_hidden(param))
			continue;
		parg = clish_pargv__get_parg(pargv, i);
		tmp = clish_parg__get_value(parg);
		space = strchr(tmp, ' ');
		if (line)
			lub_string_cat(&line, " ");
		if (space)
			lub_string_cat(&line, "\\\"");
		lub_string_cat(&line, tmp);
		if (space)
			lub_string_cat(&line, "\\\"");
	}

	return line;
}

/*--------------------------------------------------------- */
char *clish_variable__get_line(const clish_command_t * cmd, clish_pargv_t * pargv)
{
	char *line = NULL;
	char *params = NULL;

	lub_string_cat(&line, clish_command__get_name(
		clish_command__get_cmd(cmd)));

	if (!pargv)
		return line;

	params = clish_variable__get_params(cmd, pargv);
	if (params) {
		lub_string_cat(&line, " ");
		lub_string_cat(&line, params);
	}
	lub_string_free(params);

	return line;
}

/*--------------------------------------------------------- */
char *clish_variable__get_value(const char *name, const char *viewid,
	const clish_command_t * cmd, clish_pargv_t * pargv)
{
	context_t context;

	/* setup the context */
	context.viewid = viewid;
	context.cmd = cmd;
	context.pargv = pargv;

	return context_retrieve(&context, name);
}

/*--------------------------------------------------------- */
