/** @file khelper.h
 * @brief Macros for simplifying to write subsystem's service functions.
 */

#ifndef _khelper_h
#define _khelper_h

#include <faux/faux.h>
#include <faux/str.h>

// Function to get value from structure by name
#define _KGET(obj, type, name) \
	type obj##_##name(const obj##_t *inst)
#define KGET(obj, type, name) \
	_KGET(obj, type, name) { \
		assert(inst); \
		return inst->name; \
	}

#define _KGET_STR(obj, name) \
	_KGET(obj, const char *, name)
#define KGET_STR(obj, name) \
	KGET(obj, const char *, name)

#define _KGET_BOOL(obj, name) \
	_KGET(obj, bool_t, name)
#define KGET_BOOL(obj, name) \
	KGET(obj, bool_t, name)

// Function to set value to structure by name
#define _KSET(obj, type, name) \
	bool_t obj##_set_##name(obj##_t *inst, type val)
#define KSET(obj, type, name) \
	_KSET(obj, type, name) { \
		assert(inst); \
		inst->name = val; \
		return BOOL_TRUE; \
	}

#define _KSET_ONCE(obj, type, name) \
	_KSET(obj, type, name)
#define KSET_ONCE(obj, type, name) \
	_KSET_ONCE(obj, type, name) { \
		assert(inst); \
		assert(!inst->name); \
		inst->name = val; \
	}

#define _KSET_STR(obj, name) \
	_KSET(obj, const char *, name)
#define KSET_STR(obj, name) \
	_KSET_STR(obj, name) { \
		assert(inst); \
		faux_str_free(inst->name); \
		inst->name = faux_str_dup(val); \
		return BOOL_TRUE; \
	}

#define _KSET_STR_ONCE(obj, name) \
	_KSET_STR(obj, name)
#define KSET_STR_ONCE(obj, name) \
	_KSET_STR_ONCE(obj, name) { \
		assert(inst); \
		assert(!inst->name); \
		inst->name = faux_str_dup(val); \
		return BOOL_TRUE; \
	}

#define _KSET_BOOL(obj, name) \
	_KSET(obj, bool_t, name)
#define KSET_BOOL(obj, name) \
	KSET(obj, bool_t, name)

#endif // _khelper_h
