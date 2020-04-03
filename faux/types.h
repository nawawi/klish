/** @file types.h
 * @brief Additional usefull data types
 */

#ifndef _faux_types_h
#define _faux_types_h


/**
 * A standard boolean type. The possible values are
 * BOOL_FALSE and BOOL_TRUE.
 */
typedef enum {
	BOOL_FALSE = 0,
	BOOL_TRUE = 1
} bool_t;


/** @def C_DECL_BEGIN
 * This macro can be used instead standard preprocessor
 * directive like this:
 * @code
 * #ifdef __cplusplus
 * extern "C" {
 * #endif
 *
 * int foobar(void);
 *
 * #ifdef __cplusplus
 * }
 * #endif
 * @endcode
 * It make linker to use C-style linking for functions.
 * Use C_DECL_BEGIN before functions declaration and C_DECL_END
 * after declaration:
 * @code
 * C_DECL_BEGIN
 *
 * int foobar(void);
 *
 * C_DECL_END
 * @endcode
 */

/** @def C_DECL_END
 * See the macro C_DECL_BEGIN.
 * @sa C_DECL_BEGIN
 */

#ifdef __cplusplus
#define C_DECL_BEGIN extern "C" {
#define C_DECL_END }
#else
#define C_DECL_BEGIN
#define C_DECL_END
#endif


#endif /* _faux_types_h */
