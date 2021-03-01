/** @file khelper.h
 * @brief Macros for simplifying to write subsystem's service functions.
 */

#ifndef _khelper_h
#define _khelper_h

#include <faux/faux.h>
#include <faux/str.h>


// Function to get value from structure by name
#define _KGET(obj, type, name) \
	type k##obj##_##name(const k##obj##_t *inst)
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
	bool_t k##obj##_set_##name(k##obj##_t *inst, type val)
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
		if (inst->name) { \
			if (NULL == val) \
				return BOOL_FALSE; \
			if (strcmp(inst->name, val) == 0) \
				return BOOL_TRUE; \
			return BOOL_FALSE; \
		} \
		inst->name = faux_str_dup(val); \
		return BOOL_TRUE; \
	}

#define _KSET_BOOL(obj, name) \
	_KSET(obj, bool_t, name)
#define KSET_BOOL(obj, name) \
	KSET(obj, bool_t, name)

// Function to add object to list
#define _KADD_NESTED(obj, nested) \
	bool_t k##obj##_add_##nested(k##obj##_t *inst, k##nested##_t *subobj)
#define KADD_NESTED(obj, nested) \
	_KADD_NESTED(obj, nested) { \
	assert(inst); \
	if (!inst) \
		return BOOL_FALSE; \
	assert(subobj); \
	if (!subobj) \
		return BOOL_FALSE; \
	if (!faux_list_add(inst->nested##s, subobj)) \
		return BOOL_FALSE; \
	return BOOL_TRUE; \
}

#define _KFIND_NESTED(obj, nested) \
	k##nested##_t *k##obj##_find_##nested(const k##obj##_t *inst, const char *str_key)
#define KFIND_NESTED(obj, nested) \
	_KFIND_NESTED(obj, nested) { \
	assert(inst); \
	if (!inst) \
		return NULL; \
	assert(str_key); \
	if (!str_key) \
		return NULL; \
	return (k##nested##_t *)faux_list_kfind(inst->nested##s, str_key); \
}

// Compare functions. For lists
#define _KCMP_NESTED(obj, nested, field) \
	int k##obj##_##nested##_compare(const void *first, const void *second)
#define KCMP_NESTED(obj, nested, field) \
	_KCMP_NESTED(obj, nested, field) { \
	const k##nested##_t *f = (const k##nested##_t *)first; \
	const k##nested##_t *s = (const k##nested##_t *)second; \
	return strcmp(k##nested##_##field(f), k##nested##_##field(s)); \
}

#define _KCMP_NESTED_BY_KEY(obj, nested, field) \
	int k##obj##_##nested##_kcompare(const void *key, const void *list_item)
#define KCMP_NESTED_BY_KEY(obj, nested, field) \
	_KCMP_NESTED_BY_KEY(obj, nested, field) { \
	const char *f = (const char *)key; \
	const k##nested##_t *s = (const k##nested##_t *)list_item; \
	return strcmp(f, k##nested##_##field(s)); \
}


#endif // _khelper_h
