/** @file base.c
 * @brief Base faux functions.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "faux/faux.h"


/** Portable implementation of free() function.
 *
 * The POSIX standard says the free() must check pointer for NULL value
 * and must not try to free NULL pointer. I.e. it do nothing in a case of NULL.
 * The Linux glibc do so. But some non-fully POSIX compatible systems don't.
 * For these systems our implementation of free() must check for NULL itself.
 *
 * For now function simply call free() but it can be changed while working on
 * portability.
 *
 * @param [in] ptr Memory pointer to free.
 * @sa free()
 */
void faux_free(void *ptr) {

#if 0
	if (ptr)
#endif
	free(ptr);
}


/** Portable implementation of malloc() function.
 *
 * The faux library implements its own free() function (called faux_free()) so
 * it's suitable to implement their own complementary malloc() function.
 * The behaviour when the size is 0 must be strictly defined. This function
 * will assert() and return NULL when size is 0.
 *
 * @param [in] size Memory size to allocate.
 * @return Allocated memory or NULL on error.
 * @sa malloc()
 */
void *faux_malloc(size_t size) {

	assert(size != 0);
	if (0 == size)
		return NULL;

	return malloc(size);
}

/** Portable implementation of bzero().
 *
 * The POSIX standard says the bzero() is legacy now. It recommends to use
 * memset() instead it. But bzero() is easier to use. So bzero() can be
 * implemented using memset().
 *
 * @param [in] ptr Pointer
 * @param [in] size Size of memory (in bytes) to zero it.
 * @sa bzero()
 */
void faux_bzero(void *ptr, size_t size) {

	memset(ptr, '\0', size);
}


/** The malloc() implementation with writing zeroes to allocated buffer.
 *
 * The POSIX defines calloc() function to allocate memory and write zero bytes
 * to the whole buffer. But calloc() has strange set of arguments. This function
 * is simply combination of malloc() and bzero().
 *
 * @param [in] size Memory size to allocate.
 * @return Allocated zeroed memory or NULL on error.
 */
void *faux_zmalloc(size_t size) {

	void *ptr = NULL;

	ptr = faux_malloc(size);
	if (ptr)
		faux_bzero(ptr, size);

	return ptr;
}
