/* Macros for simplifying to write subsystem's service functions */

#ifndef _clish_macros_h
#define _clish_macros_h

#include <assert.h>

/* Function to get value from structure by name */
#define _CLISH_GET(obj, type, name) \
	type clish_##obj##__get_##name(const clish_##obj##_t *obj)
#define CLISH_GET(obj, type, name) \
	_CLISH_GET(obj, type, name) { \
		assert(obj); \
		return obj->name; \
	}

#define CLISH_SET(obj_type, field_type, field_name, value)




#endif // _clish_macros_h
