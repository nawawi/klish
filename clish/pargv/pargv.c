/*
 * paramv.c
 */
#include "private.h"
#include "lub/string.h"
#include "lub/argv.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

/*--------------------------------------------------------- */
/*
 * Search for the specified parameter and return its value
 */
static clish_parg_t *find_parg(clish_pargv_t * this, const char *name)
{
	unsigned i;
	clish_parg_t *result = NULL;

	/* scan the parameters in this instance */
	for (i = 0; i < this->pargc; i++) {
		clish_parg_t *parg = this->pargv[i];
		const char *pname = clish_param__get_name(parg->param);

		if (0 == strcmp(pname, name)) {
			result = parg;
			break;
		}
	}
	return result;
}

/*--------------------------------------------------------- */
void
clish_pargv_insert(clish_pargv_t * this,
		   const clish_param_t * param, const char *value)
{
	clish_parg_t *parg = find_parg(this, clish_param__get_name(param));

	if (NULL != parg) {
		/* release the current value */
		lub_string_free(parg->value);
	} else {
		size_t new_size = ((this->pargc + 1) * sizeof(clish_parg_t *));
		clish_parg_t **tmp;

		/* resize the parameter vector */
		tmp = realloc(this->pargv, new_size);
		this->pargv = tmp;
		/* insert reference to the parameter */
		parg = malloc(sizeof(*parg));
		this->pargv[this->pargc++] = parg;
		parg->param = param;
	}
	parg->value = lub_string_dup(value);
}

/*--------------------------------------------------------- */
static void set_defaults(clish_pargv_t * this, const clish_command_t * cmd)
{
	unsigned index = 0;
	const clish_param_t *param;

	/* scan through all the parameters for this command */
	while ((param = clish_command__get_param(cmd, index++))) {
		const char *defval = clish_param__get_default(param);
		if (NULL != defval) {
			if ('\0' != *defval) {
				/* add the translated value to the vector */
				char *translated =
				    clish_ptype_translate(clish_param__get_ptype
							  (param),
							  defval);
				clish_pargv_insert(this, param, translated);
				lub_string_free(translated);
			} else {
				/* insert the empty default */
				clish_pargv_insert(this, param, defval);
			}

		}
	}
}

/*--------------------------------------------------------- */
clish_pargv_status_t
clish_pargv_parse(clish_pargv_t * this,
		const clish_command_t * cmd,
		clish_paramv_t * paramv,
		const lub_argv_t * argv,
		unsigned *idx, clish_pargv_t * last, unsigned need_index)
{
	unsigned start = *idx;
	unsigned argc = lub_argv__get_count(argv);
	unsigned index = 0;
	unsigned nopt_index = 0;
	clish_param_t *nopt_param = NULL;
	unsigned i;
	clish_pargv_status_t retval;
	unsigned paramc = clish_paramv__get_count(paramv);

	assert(this);

	while (index < paramc) {
		const char *arg;
		clish_param_t *param = clish_paramv__get_param(paramv,index);
		clish_param_t *cparam = NULL;
		int is_switch = 0;

		/* Use real arg or PARAM's default value as argument */
		if (*idx >= argc)
			arg = clish_param__get_default(param);
		else
			arg = lub_argv__get_arg(argv, *idx);

		/* Is parameter in "switch" mode? */
		if (CLISH_PARAM_SWITCH == clish_param__get_mode(param))
			is_switch = 1;

		/* Add param for help and completion */
		if (last && (*idx == need_index) &&
			(NULL == clish_pargv_find_arg(this, clish_param__get_name(param)))) {
			if (is_switch) {
				clish_paramv_t *rec_paramv = clish_param__get_paramv(param);
				unsigned rec_paramc = clish_param__get_param_count(param);
				for (i = 0; i < rec_paramc; i++) {
					cparam = clish_param__get_param(param, i);
					if (!cparam)
						break;
					if (CLISH_PARAM_SUBCOMMAND ==
						clish_param__get_mode(cparam)) {
						const char *pname =
							clish_param__get_name(cparam);
						if (!arg || (arg && 
							(pname == lub_string_nocasestr(pname,
							arg))))
							clish_pargv_insert(last, 
								cparam, arg);
					} else {
						clish_pargv_insert(last,
							cparam, arg);
					}
				}
			} else {
				if (CLISH_PARAM_SUBCOMMAND ==
					clish_param__get_mode(param)) {
					const char *pname =
					    clish_param__get_name(param);
					if (!arg || (arg &&
						(pname == lub_string_nocasestr(pname, arg))))
						clish_pargv_insert(last, param, arg);
				} else {
					clish_pargv_insert(last, param, arg);
				}
			}
		}

		/* Set parameter value */
		if (NULL != param) {
			char *validated = NULL;
			clish_paramv_t *rec_paramv =
			    clish_param__get_paramv(param);
			unsigned rec_paramc =
			    clish_param__get_param_count(param);

			/* Save the index of last non-option parameter
			 * to restore index if the optional parameters
			 * will be used.
			 */
			if (BOOL_TRUE != clish_param__get_optional(param)) {
				nopt_param = param;
				nopt_index = index;
			}

			/* Validate the current parameter. */
			if (NULL != clish_pargv_find_arg(this, clish_param__get_name(param))) {
				/* Duplicated parameter */
				validated = NULL;
			} else if (is_switch) {
				for (i = 0; i < rec_paramc; i++) {
					cparam =
					    clish_param__get_param(param, i);
					if (!cparam)
						break;
					if (validated =
					    arg ? clish_param_validate(cparam,
								       arg) :
					    NULL) {
						rec_paramv =
						    clish_param__get_paramv
						    (cparam);
						rec_paramc =
						    clish_param__get_param_count
						    (cparam);
						break;
					}
				}
			} else {
				validated =
				    arg ? clish_param_validate(param,
							       arg) : NULL;
			}

			if (validated) {
				/* add (or update) this parameter */
				if (is_switch) {
					clish_pargv_insert(this, param,
						clish_param__get_name(cparam));
					clish_pargv_insert(this, cparam,
						validated);
				} else {
					clish_pargv_insert(this, param,
						validated);
				}
				lub_string_free(validated);

				/* Next command line argument */
				(*idx)++;

				/* Walk through the nested parameters */
				if (rec_paramc) {
					retval = clish_pargv_parse(this, NULL, rec_paramv,
						argv, idx, last, need_index);
					if (clish_LINE_OK != retval)
						return retval;
				}

				/* Choose the next parameter */
				if (BOOL_TRUE == clish_param__get_optional(param)) {
					if (nopt_param)
						index = nopt_index + 1;
					else
						index = 0;
				} else {
					index++;
				}

			} else {
				/* Choose the next parameter if current
				 * is not validated.
				 */
				if (BOOL_TRUE ==
					clish_param__get_optional(param))
					index++;
				else
					return clish_BAD_PARAM;
			}
		} else {
			return clish_BAD_PARAM;
		}
	}

	/* Check for non-optional parameters without values */
	if ((*idx >= argc) && (index < paramc)) {
		unsigned j = index;
		const clish_param_t *param;
		while (j < paramc) {
			param = clish_paramv__get_param(paramv, j++);
			if (BOOL_TRUE != clish_param__get_optional(param))
				return clish_BAD_PARAM;
		}
	}

	/* If the number of arguments is bigger than number of
	 * params than it's a args. So generate the args entry
	 * in the list of completions.
	 */
	if (last &&
			cmd && clish_command__get_args(cmd) &&
			(clish_pargv__get_count(last) == 0) &&
			(*idx <= argc) && (index >= paramc)) {
		clish_pargv_insert(last, clish_command__get_args(cmd), "");
	}

	/*
	 * if we've satisfied all the parameters we can now construct
	 * an 'args' parameter if one exists
	 */
	if (cmd && clish_command__get_args(cmd) &&
			(*idx < argc) && (index >= paramc)) {
		const char *arg = lub_argv__get_arg(argv, *idx);
		const clish_param_t *param = clish_command__get_args(cmd);
		char *args = NULL;

		/* 
		 * put all the argument into a single string 
		 */
		while (NULL != arg) {
			bool_t quoted = lub_argv__get_quoted(argv, *idx);
			if (BOOL_TRUE == quoted) {
				lub_string_cat(&args, "\"");
			}
			/* place the current argument in the string */
			lub_string_cat(&args, arg);
			if (BOOL_TRUE == quoted) {
				lub_string_cat(&args, "\"");
			}
			(*idx)++;
			arg = lub_argv__get_arg(argv, *idx);
			if (NULL != arg) {
				/* add a space if there are more arguments */
				lub_string_cat(&args, " ");
			}
		}
		/* add (or update) this parameter */
		clish_pargv_insert(this, param, args);
		lub_string_free(args);
	}

	return clish_LINE_OK;
}

/*--------------------------------------------------------- */
static clish_pargv_status_t
clish_pargv_init(clish_pargv_t * this,
		 const clish_command_t * cmd, const lub_argv_t * argv)
{
	unsigned idx = lub_argv_wordcount(clish_command__get_name(cmd));

	this->pargc = 0;

	return clish_pargv_parse(this, cmd,
		clish_command__get_paramv(cmd),
		argv, &idx, NULL, 0);
}

/*--------------------------------------------------------- */
clish_pargv_t *clish_pargv_create(void)
{
	clish_pargv_t *tmp;

	tmp = malloc(sizeof(clish_pargv_t));
	tmp->pargc = 0;
	tmp->pargv = NULL;

	return tmp;
}

/*--------------------------------------------------------- */
clish_pargv_t *clish_pargv_new(const clish_command_t * cmd,
			       const char *line,
			       size_t offset, clish_pargv_status_t * status)
{
	clish_pargv_t *this;
	lub_argv_t *argv = lub_argv_new(line, offset);

	this = clish_pargv_create();
	if (NULL != this) {
		*status = clish_pargv_init(this, cmd, argv);
		switch (*status) {
		case clish_LINE_OK:
			break;
		case clish_BAD_CMD:
		case clish_BAD_PARAM:
		case clish_BAD_HISTORY:
			/* commit suicide */
			clish_pargv_delete(this);
			this = NULL;
			break;
		}
	}

	/* cleanup */
	lub_argv_delete(argv);

	return this;
}

/*--------------------------------------------------------- */
static void clish_pargv_fini(clish_pargv_t * this)
{
	unsigned i;

	/* cleanup time */
	for (i = 0; i < this->pargc; i++) {
		lub_string_free(this->pargv[i]->value);
		this->pargv[i]->value = NULL;
		free(this->pargv[i]);
	}
	free(this->pargv);
}

/*--------------------------------------------------------- */
void clish_pargv_delete(clish_pargv_t * this)
{
	clish_pargv_fini(this);
	free(this);
}

/*--------------------------------------------------------- */
unsigned clish_pargv__get_count(clish_pargv_t * this)
{
	return this->pargc;
}

/*--------------------------------------------------------- */
clish_parg_t *clish_pargv__get_parg(clish_pargv_t * this, unsigned index)
{
	if (index > this->pargc)
		return NULL;
	return this->pargv[index];
}

/*--------------------------------------------------------- */
const clish_param_t *clish_pargv__get_param(clish_pargv_t * this,
					    unsigned index)
{
	clish_parg_t *tmp;

	if (index >= this->pargc)
		return NULL;
	tmp = this->pargv[index];
	return tmp->param;
}

/*--------------------------------------------------------- */
const char *clish_parg__get_value(const clish_parg_t * this)
{
	return this->value;
}

/*--------------------------------------------------------- */
const char *clish_parg__get_name(const clish_parg_t * this)
{
	return clish_param__get_name(this->param);
}

/*--------------------------------------------------------- */
const clish_ptype_t *clish_parg__get_ptype(const clish_parg_t * this)
{
	return clish_param__get_ptype(this->param);
}

/*--------------------------------------------------------- */
const clish_parg_t *clish_pargv_find_arg(clish_pargv_t * this, const char *name)
{
	return find_parg(this, name);
}

/*--------------------------------------------------------- */
