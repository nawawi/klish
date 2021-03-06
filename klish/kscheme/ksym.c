#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/str.h>
#include <faux/list.h>
#include <faux/conv.h>
#include <faux/error.h>
#include <klish/khelper.h>
#include <klish/ksym.h>


struct ksym_s {
	char *name;
	const void *fn;
};


// Simple methods

// Name
KGET_STR(sym, name);
KSET_STR_ONCE(sym, name);

// Fn (function)
KGET(sym, const void *, fn);
KSET(sym, const void *, fn);


ksym_t *ksym_new(const char *name)
{
	ksym_t *sym = NULL;

	if (faux_str_is_empty(name))
		return NULL;

	sym = faux_zmalloc(sizeof(*sym));
	assert(sym);
	if (!sym)
		return NULL;

	// Initialize
	sym->name = faux_str_dup(name);
	sym->fn = NULL;

	return sym;
}


void ksym_free(ksym_t *sym)
{
	if (!sym)
		return;

	faux_str_free(sym->name);

	faux_free(sym);
}
