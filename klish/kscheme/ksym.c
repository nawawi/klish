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
	ksym_fn function;
	tri_t permanent; // Dry-run option has no effect for permanent sym
	tri_t sync; // Don't fork before sync sym execution
	bool_t silent; // Silent syn doesn't have stdin, stdout, stderr
};


// Simple methods

// Name
KGET_STR(sym, name);
KSET_STR_ONCE(sym, name);

// Function
KGET(sym, ksym_fn, function);
KSET(sym, ksym_fn, function);

// Permanent
KGET(sym, tri_t, permanent);
KSET(sym, tri_t, permanent);

// Sync
KGET(sym, tri_t, sync);
KSET(sym, tri_t, sync);

// Silent
KGET(sym, bool_t, silent);
KSET(sym, bool_t, silent);


ksym_t *ksym_new(const char *name, ksym_fn function)
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
	sym->function = function;
	sym->permanent = TRI_UNDEFINED;
	sym->sync = TRI_UNDEFINED;
	sym->silent = BOOL_FALSE;

	return sym;
}


ksym_t *ksym_new_ext(const char *name, ksym_fn function,
	tri_t permanent, tri_t sync)
{
	ksym_t *sym = NULL;

	sym = ksym_new(name, function);
	if (!sym)
		return NULL;

	ksym_set_permanent(sym, permanent);
	ksym_set_sync(sym, sync);

	return sym;
}


void ksym_free(ksym_t *sym)
{
	if (!sym)
		return;

	faux_str_free(sym->name);

	faux_free(sym);
}
