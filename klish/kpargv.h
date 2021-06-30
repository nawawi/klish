/** @file kpargv.h
 *
 * @brief Parsed ARGuments Vector
 */

#ifndef _klish_kpargv_h
#define _klish_kpargv_h

#include <faux/list.h>
#include <klish/kentry.h>


typedef enum {
	KPARSE_NONE,
	KPARSE_OK,
	KPARSE_INPROGRESS,
	KPARSE_NOTFOUND,
	KPARSE_INCOMPLETED,
	KPARSE_ILLEGAL,
	KPARSE_ERROR,
} kpargv_status_e;


typedef struct kpargv_s kpargv_t;
typedef struct kparg_s kparg_t;

typedef faux_list_node_t kpargv_pargs_node_t;


C_DECL_BEGIN

// Parg

kparg_t *kparg_new(kentry_t *entry, const char *value);
void kparg_free(kparg_t *parg);

kentry_t *kparg_entry(const kparg_t *parg);
bool_t kparg_set_value(kparg_t *parg, const char *value);
const char *kparg_value(const kparg_t *parg);

// Pargv

kpargv_t *kpargv_new();
void kpargv_free(kpargv_t *pargv);

// Status
kpargv_status_e kpargv_status(const kpargv_t *pargv);
bool_t kpargv_set_status(kpargv_t *pargv, kpargv_status_e status);
const char *kpargv_status_str(const kpargv_t *pargv);
// Level
size_t kpargv_level(const kpargv_t *pargv);
bool_t kpargv_set_level(kpargv_t *pargv, size_t level);
// Command
const kentry_t *kpargv_command(const kpargv_t *pargv);
bool_t kpargv_set_command(kpargv_t *pargv, const kentry_t *command);
// Continuable
bool_t kpargv_continuable(const kpargv_t *pargv);
bool_t kpargv_set_continuable(kpargv_t *pargv, bool_t continuable);
// Pargs
faux_list_t *kpargv_pargs(const kpargv_t *pargv);
ssize_t kpargv_pargs_len(const kpargv_t *pargv);
bool_t kpargv_pargs_is_empty(const kpargv_t *pargv);
bool_t kpargv_add_parg(kpargv_t *pargv, kparg_t *parg);
kpargv_pargs_node_t *kpargv_pargs_iter(const kpargv_t *pargv);
kparg_t *kpargv_pargs_each(kpargv_pargs_node_t **iter);
kparg_t *kpargv_pargs_last(const kpargv_t *pargv);
kparg_t *kpargv_entry_exists(const kpargv_t *pargv, const void *entry);

C_DECL_END

#endif // _klish_kpargv_h
