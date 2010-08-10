/*
 * clish_access_callback.c
 *
 *
 * callback hook to check whether the current user is a 
 * member of the specified group (access string)
 */
#include <assert.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <grp.h>

#include "private.h"
/*--------------------------------------------------------- */
bool_t clish_access_callback(const clish_shell_t * shell, const char *access)
{
	bool_t allowed = BOOL_FALSE;

	assert(access);
	/* There is an access restricion on this element */
	int num_groups;
#define MAX_GROUPS 10
	gid_t group_list[MAX_GROUPS];
	int i;

	/* assume the worst */
	allowed = BOOL_FALSE;

	/* get the groups for the current user */
	num_groups = getgroups(MAX_GROUPS, group_list);
	assert(num_groups != -1);

	/* now check these against the access provided */
	for (i = 0; i < num_groups; i++) {
		struct group *ptr = getgrgid(group_list[i]);
		if (0 == strcmp(ptr->gr_name, access)) {
			/* The current user is permitted to use this command */
			allowed = BOOL_TRUE;
			break;
		}
	}
	return allowed;
}

/*--------------------------------------------------------- */
