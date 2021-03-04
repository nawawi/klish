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


static ksym_t *ksym_new_empty(void)
{
	ksym_t *sym = NULL;

	sym = faux_zmalloc(sizeof(*sym));
	assert(sym);
	if (!sym)
		return NULL;

	// Initialize
	sym->name = NULL;
	sym->fn = NULL;

	return sym;
}


ksym_t *ksym_new(ksym_error_e *error)
{
	ksym_t *sym = NULL;

	sym = ksym_new_empty();
	assert(sym);
	if (!sym) {
		if (error)
			*error = KSYM_ERROR_ALLOC;
		return NULL;
	}

	return sym;
}


void ksym_free(ksym_t *sym)
{
	if (!sym)
		return;

	faux_str_free(sym->name);

	faux_free(sym);
}


const char *ksym_strerror(ksym_error_e error)
{
	const char *str = NULL;

	switch (error) {
	case KSYM_ERROR_OK:
		str = "Ok";
		break;
	case KSYM_ERROR_INTERNAL:
		str = "Internal error";
		break;
	case KSYM_ERROR_ALLOC:
		str = "Memory allocation error";
		break;
	default:
		str = "Unknown error";
		break;
	}

	return str;
}
