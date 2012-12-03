/*
 * plugin_dump.c
 */
#include "private.h"
#include "lub/dump.h"
#include "lub/list.h"
#include "clish/plugin.h"

/*--------------------------------------------------------- */
void clish_sym_dump(const clish_sym_t *this)
{
	lub_dump_printf("sym(%p)\n", this);

	lub_dump_indent();
	lub_dump_printf("name : %s\n", this->name);
	lub_dump_printf("func : %p\n", this->func);
	lub_dump_undent();
}

/*--------------------------------------------------------- */
void clish_plugin_dump(const clish_plugin_t *this)
{
	lub_list_node_t *iter;
	clish_sym_t *sym;

	lub_dump_printf("plugin(%p)\n", this);
	lub_dump_printf("name  : %s\n", this->name);
	lub_dump_printf("file  : %s\n", this->file);
	lub_dump_printf("dlhan : %p\n", this->dlhan);
	lub_dump_indent();
	/* Iterate child elements */
	for(iter = lub_list__get_head(this->syms);
		iter; iter = lub_list_node__get_next(iter)) {
		sym = (clish_sym_t *)lub_list_node__get_data(iter);
		clish_sym_dump(sym);
	}
	lub_dump_undent();
}

/*--------------------------------------------------------- */
