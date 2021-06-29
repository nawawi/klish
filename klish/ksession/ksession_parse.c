/** @file ksession_parse.c
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <faux/argv.h>
#include <klish/khelper.h>
#include <klish/kview.h>
#include <klish/kscheme.h>
#include <klish/kpath.h>
#include <klish/kpargv.h>
#include <klish/ksession.h>


static bool_t ksession_validate_arg(kentry_t *entry, const char *arg)
{
	const char *str = NULL;

	assert(entry);
	if (!entry)
		return BOOL_FALSE;
	assert(arg);
	if (!arg)
		return BOOL_FALSE;

	// Temporary test code that implements COMMAND i.e. it compares argument
	// to ENTRY's 'name' or 'value'. Later it will be removed by real code.
	str = kentry_value(entry);
	if (!str)
		str = kentry_name(entry);
	if (faux_str_casecmp(str, arg) == 0)
			return BOOL_TRUE;

	return BOOL_FALSE;
}


static kparse_status_e ksession_parse_arg(kentry_t *current_entry,
	faux_argv_node_t **argv_iter, kpargv_t *pargv)
{
	kentry_t *entry = current_entry;
	kentry_mode_e mode = KENTRY_MODE_NONE;
	kparse_status_e retcode = KPARSE_NOTFOUND; // For ENTRY itself
	kparse_status_e rc = KPARSE_NOTFOUND; // For nested ENTRYs

	assert(current_entry);
	if (!current_entry)
		return KPARSE_ERROR;
	assert(argv_iter);
	if (!argv_iter)
		return KPARSE_ERROR;
	assert(pargv);
	if (!pargv)
		return KPARSE_ERROR;

	// If all arguments are resolved already then return INCOMPLETED
	assert(*argv_iter);
	if (!*argv_iter)
		return KPARSE_INCOMPLETED;

	// Is entry candidate to resolve current arg?
	// Container can't be a candidate.
	if (!kentry_container(entry)) {
		const char *current_arg = faux_argv_current(*argv_iter);
		if (ksession_validate_arg(entry, current_arg)) {
			kparg_t *parg = kparg_new(entry, current_arg);
			kpargv_add_parg(pargv, parg);
			faux_argv_each(argv_iter); // Next argument
			retcode = KPARSE_INPROGRESS;
		} else {
			// It's not a container and is not validated so
			// no chance to find anything here.
			return KPARSE_NOTFOUND;
		}
	}

	// ENTRY has no nested ENTRYs so return
	if (kentry_entrys_is_empty(entry))
		return retcode;

	// Walk through the nested entries
	mode = kentry_mode(entry);
	if (KENTRY_MODE_EMPTY == mode)
		return retcode;

	// SWITCH
	// Entries within SWITCH can't has 'min'/'max' else than 1.
	// So these attributes will be ignored. Note SWITCH itself can have
	// 'min'/'max'.
	if (KENTRY_MODE_SWITCH == mode) {
		kentry_entrys_node_t *iter = kentry_entrys_iter(entry);
		kentry_t *nested = NULL;
		while ((nested = kentry_entrys_each(&iter))) {
			rc = ksession_parse_arg(nested, argv_iter, pargv);
			// Any variant of error or INPROGRESS
			if (rc != KPARSE_NOTFOUND)
				break;
		}

	// SEQUENCE
	} else if (KENTRY_MODE_SEQUENCE == mode) {
		kentry_entrys_node_t *iter = kentry_entrys_iter(entry);
		kentry_entrys_node_t *saved_iter = iter;
		kentry_t *nested = NULL;

		while ((nested = kentry_entrys_each(&iter))) {
			kparse_status_e nrc = KPARSE_NOTFOUND;
			size_t num = 0;
			if (kpargv_entry_exists(pargv, nested))
				continue;
			for (num = 0; num < kentry_max(nested); num++) {
				nrc = ksession_parse_arg(nested, argv_iter, pargv);
				if (nrc != KPARSE_INPROGRESS)
					break;
			}
			if ((nrc != KPARSE_INPROGRESS) && (nrc != KPARSE_NOTFOUND)) {
				rc = nrc;
				break;
			}
			// Not found all mandatory instances (NOTFOUND)
			if (num < kentry_min(nested)) {
				rc = KPARSE_NOTFOUND;
				break;
			}
			// It's not an error if optional parameter is absend
			rc = KPARSE_INPROGRESS;

			// Mandatory or ordered parameter
			if ((kentry_min(nested) > 0) ||
				kentry_order(nested))
				saved_iter = iter;
			iter = saved_iter;
		}
	
	}

	// When ENTRY (not container) is found but mandatory nested ENTRY is
	// not resolved. It's inconsistent. So NOTFOUND is not suitable in
	// this case.
	if ((KPARSE_NOTFOUND == rc) && (KPARSE_INPROGRESS == retcode))
		return KPARSE_ILLEGAL;

	return rc;
}


kparse_status_e ksession_parse_line(ksession_t *session, const char *line,
	kpargv_t **parsed_argv)
{
	faux_argv_t *argv = NULL;
	faux_argv_node_t *argv_iter = NULL;
	kentry_t *current_entry = NULL;
	kpargv_t *pargv = NULL;
	kparse_status_e pstatus = KPARSE_NONE;

	if (parsed_argv)
		*parsed_argv = NULL;
	assert(session);
	if (!session)
		return KPARSE_ERROR;
	assert(line);
	if (!line)
		return KPARSE_ERROR;

	// Split line to arguments
	argv = faux_argv_new();
	assert(argv);
	if (!argv)
		return KPARSE_ERROR;
	if (faux_argv_parse(argv, line) < 0) {
		faux_argv_free(argv);
		return KPARSE_ERROR;
	}
printf("AAAAAAAAAAA %ld\n", faux_argv_len(argv));
	argv_iter = faux_argv_iter(argv);

	current_entry = klevel_entry(kpath_current(ksession_path(session)));
	pargv = kpargv_new();
	assert(pargv);

	pstatus = ksession_parse_arg(current_entry, &argv_iter, pargv);
	// It's a higher level of parsing, so some statuses can have different
	// meanings
/*	if (KPARSE_NONE == pstatus)
		pstatus = KPARSE_ERROR; // Strange case
	else if (KPARSE_INPROGRESS == pstatus) {
		if (NULL == argv_iter) // All args are parsed
			pstatus = KPARSE_OK;
		else
			pstatus = KPARSE_ILLEGAL; // Additional not parsable args
	}
*/
printf("KKKKKKKKK %ld\n", kpargv_pargs_len(pargv));

	if (kpargv_pargs_is_empty(pargv)) {
		kpargv_free(pargv);
		pargv = NULL;
	}
	*parsed_argv = pargv;

	return pstatus;
}
