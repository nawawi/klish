/* Macros for simplifying to write subsystem's service functions */

#ifndef _clish_macros_h
#define _clish_macros_h

#include <assert.h>

/* Function to get value from structure by name */
#define _CLISH_GET(obj, type, name) \
	type clish_##obj##__get_##name(const clish_##obj##_t *inst)
#define CLISH_GET(obj, type, name) \
	_CLISH_GET(obj, type, name) { \
		assert(inst); \
		return inst->name; \
	}
#define _CLISH_GET_STR(obj, name) \
	_CLISH_GET(obj, const char *, name)
#define CLISH_GET_STR(obj, name) \
	CLISH_GET(obj, const char *, name)

/* Function to set value to structure by name */
#define _CLISH_SET(obj, type, name) \
	void clish_##obj##__set_##name(clish_##obj##_t *inst, type val)
#define CLISH_SET(obj, type, name) \
	_CLISH_SET(obj, type, name) { \
		assert(inst); \
		inst->name = val; \
	}
#define _CLISH_SET_ONCE(obj, type, name) \
	_CLISH_SET(obj, type, name)
#define CLISH_SET_ONCE(obj, type, name) \
	_CLISH_SET_ONCE(obj, type, name) { \
		assert(inst); \
		assert(!inst->name); \
		inst->name = val; \
	}
#define _CLISH_SET_STR(obj, name) \
	_CLISH_SET(obj, const char *, name)
#define CLISH_SET_STR(obj, name) \
	_CLISH_SET_STR(obj, name) { \
		assert(inst); \
		lub_string_free(inst->name); \
		inst->name = lub_string_dup(val); \
	}
#define _CLISH_SET_STR_ONCE(obj, name) \
	_CLISH_SET_STR(obj, name)
#define CLISH_SET_STR_ONCE(obj, name) \
	_CLISH_SET_STR_ONCE(obj, name) { \
		assert(inst); \
		assert(!inst->name); \
		inst->name = lub_string_dup(val); \
	}

#endif // _clish_macros_h
