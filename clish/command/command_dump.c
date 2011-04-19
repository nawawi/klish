/*
 * command_dump.c
 */

#include "lub/dump.h"
#include "clish/action.h"
#include "private.h"

/*--------------------------------------------------------- */
void clish_command_dump(const clish_command_t * this)
{
	unsigned i;
	char *cfg_op;

	lub_dump_printf("command(%p)\n", this);
	lub_dump_indent();
	lub_dump_printf("name        : %s\n", this->name);
	lub_dump_printf("text        : %s\n", this->text);
	lub_dump_printf("link        : %s\n",
		this->link ? clish_command__get_name(this->link) : "(null)");
	lub_dump_printf("alias       : %s\n", this->alias);
	lub_dump_printf("alias_view  : %s\n",
		this->alias_view ? clish_view__get_name(this->alias_view) : "(null)");
	clish_action_dump(this->action);
	lub_dump_printf("paramc      : %d\n", clish_paramv__get_count(this->paramv));
	lub_dump_printf("detail      : %s\n",
			this->detail ? this->detail : "(null)");
	switch (this->cfg_op) {
	case CLISH_CONFIG_NONE:
		cfg_op = "NONE";
		break;
	case CLISH_CONFIG_SET:
		cfg_op = "SET";
		break;
	case CLISH_CONFIG_UNSET:
		cfg_op = "UNSET";
		break;
	default:
		cfg_op = "Unknown";
		break;
	}
	lub_dump_printf("cfg_op      : %s\n", cfg_op);

	/* Get each parameter to dump their details */
	for (i = 0; i < clish_paramv__get_count(this->paramv); i++) {
		clish_param_dump(clish_command__get_param(this, i));
	}

	lub_dump_undent();
}

/*--------------------------------------------------------- */
