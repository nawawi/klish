/*
 * shell_var.c
 */

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "lub/string.h"
#include "private.h"

/*--------------------------------------------------------- */
void clish_shell_insert_var(clish_shell_t *this, clish_var_t *var)
{
	(void)lub_bintree_insert(&this->var_tree, var);
}

/*--------------------------------------------------------- */
clish_var_t *clish_shell_find_var(clish_shell_t *this, const char *name)
{
	return lub_bintree_find(&this->var_tree, name);
}

/*--------------------------------------------------------- */
const char *clish_shell__get_viewid(const clish_shell_t *this)
{
	assert(this);
	return this->viewid;
}

/*----------------------------------------------------------- */
/*
 * search the current viewid string for a variable
 */
static char *find_viewid_var(const char *name, const char *viewid)
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
	 * lint seems to equate regmatch_t[] as being of type regmatch_t !
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
static char *find_context_var(const char *name, clish_context_t *this)
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
			result = clish_shell__get_line(this);
	} else if (!lub_string_nocasecmp(name, "__params")) {
		if (this->pargv)
			result = clish_shell__get_params(this);
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
static char *find_global_var(const char *name, clish_shell_var_t vtype, clish_context_t *context)
{
	clish_shell_t *this = context->shell;
	clish_var_t *var = clish_shell_find_var(this, name);
	clish_action_t *action;
	char *value;
	char *script;
	bool_t dynamic;
	char *res = NULL;

	if (!var)
		return NULL;

	/* Try to get saved value for static var */
	dynamic = clish_var__get_dynamic(var);
	if (!dynamic) {
		char *saved = clish_var__get_saved(var);
		if (saved)
			return lub_string_dup(saved);
	}

	/* Try to expand value field */
	value = clish_var__get_value(var);
	if (value)
		res = clish_shell_expand(value, vtype, context);

	/* Try to execute ACTION */
	if (!res) {
		action = clish_var__get_action(var);
		script = clish_action__get_script(action);
		if (script) {
			char *out = NULL;
			if (!clish_shell_exec_action(action, context, &out)) {
				lub_string_free(out);
				return NULL;
			}
			res = out;
		}
	}

	/* Save value for static var */
	if (!dynamic && res)
		clish_var__set_saved(var, res);

	return res;
}

/*--------------------------------------------------------- */
/*
 * return the next segment of text from the provided string
 * segments are delimited by variables within the string.
 */
static char *expand_nextsegment(const char **string, clish_shell_var_t vtype, clish_context_t *this)
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
					char *var = clish_shell_expand_var(q, vtype, this);

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
char *clish_shell_expand(const char *str, clish_shell_var_t vtype, void *context)
{
	char *seg, *result = NULL;

	/* read each segment and extend the result */
	while ((seg = expand_nextsegment(&str, vtype, context))) {
		lub_string_cat(&result, seg);
		lub_string_free(seg);
	}

	return result;
}

/*--------------------------------------------------------- */
char *clish_shell__get_params(clish_context_t *context)
{
	clish_pargv_t *pargv = context->pargv;
	char *line = NULL;
	unsigned i, cnt;
	const clish_param_t *param;
	const clish_parg_t *parg;

	if (!pargv)
		return NULL;

	cnt = clish_pargv__get_count(pargv);
	for (i = 0; i < cnt; i++) {
		param = clish_pargv__get_param(pargv, i);
		if (clish_param__get_hidden(param))
			continue;
		parg = clish_pargv__get_parg(pargv, i);
		line = clish_shell_expand_var(clish_parg__get_name(parg),
			SHELL_VAR_ACTION, context);
	}

	return line;
}

/*--------------------------------------------------------- */
char *clish_shell__get_line(clish_context_t *context)
{
	const clish_command_t *cmd = context->cmd;
	clish_pargv_t *pargv = context->pargv;
	char *line = NULL;
	char *params = NULL;

	lub_string_cat(&line, clish_command__get_name(
		clish_command__get_cmd(cmd)));

	if (!pargv)
		return line;

	params = clish_shell__get_params(context);
	if (params) {
		lub_string_cat(&line, " ");
		lub_string_cat(&line, params);
	}
	lub_string_free(params);

	return line;
}

/*--------------------------------------------------------- */
char *clish_shell_expand_var(const char *name, clish_shell_var_t vtype, void *context)
{
	clish_context_t *con = (clish_context_t *)context;
	clish_shell_t *this;
	const clish_command_t *cmd;
	clish_pargv_t *pargv;
	char *result = NULL;
	const char *tmp = NULL;
	const char *escape_chars = NULL;
	char *string = NULL;
	assert(name);

	if (!context)
		return lub_string_dup(name);
	this = con->shell;
	cmd = con->cmd;
	pargv = con->pargv;

	/* try and substitute a parameter value */
	if (pargv) {
		const clish_parg_t *parg = clish_pargv_find_arg(pargv, name);
		/* substitute the command line value */
		if (parg) {
			char *space = NULL;
			tmp = clish_parg__get_value(parg);
			space = strchr(tmp, ' ');
			if (space) {
				char *q = NULL;
				char *tstr;
				tstr = lub_string_encode(tmp, lub_string_esc_quoted);
				lub_string_cat(&q, "\"");
				lub_string_cat(&q, tstr);
				lub_string_free(tstr);
				lub_string_cat(&q, "\"");
				tmp = string = q;
			}
		}
	}
	/* try and substitute the param's default */
	if (!tmp && cmd)
		tmp = clish_paramv_find_default(
			clish_command__get_paramv(cmd), name);
	/* try and substitute a viewId variable */
	if (!tmp && this && this->viewid)
		tmp = string = find_viewid_var(name, this->viewid);
	/* try and substitute context fixed variable */
	if (!tmp)
		tmp = string = find_context_var(name, context);
	/* try and substitute a global var value */
	if (!tmp && this)
		tmp = string = find_global_var(name, vtype, context);
	/* get the contents of an environment variable */
	if (!tmp)
		tmp = getenv(name);

	/* Escape special characters */
	if (SHELL_VAR_REGEX == vtype) {
		char *tstr;
		if (cmd)
			escape_chars = clish_command__get_regex_chars(cmd);
		if (!escape_chars)
			escape_chars = lub_string_esc_regex;
		tstr = lub_string_encode(tmp, escape_chars);
		lub_string_free(string);
		tmp = string = tstr;
	}
	escape_chars = NULL;
	if (cmd)
		escape_chars = clish_command__get_escape_chars(cmd);
	result = lub_string_encode(tmp, escape_chars);
	/* free the dynamic memory */
	lub_string_free(string);

	return result;
}

/*----------------------------------------------------------- */
