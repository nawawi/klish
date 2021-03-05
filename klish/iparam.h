/** @file iparam.h
 *
 * @brief Klish scheme's "param" entry
 */

#ifndef _klish_iparam_h
#define _klish_iparam_h


typedef struct iparam_s iparam_t;

struct iparam_s {
	char *name;
	char *help;
	char *ptype;
	iparam_t * (*params)[]; // Nested PARAMs
};

C_DECL_BEGIN

// iparam_t
char *iparam_to_text(const iparam_t *iparam, int level);

C_DECL_END

#endif // _klish_iparam_h
