/** @file kcontext_base.h
 *
 * @brief Klish base context to pass to plugin's functions
 */

#ifndef _klish_kcontext_base_h
#define _klish_kcontext_base_h

#include <faux/faux.h>

typedef struct kcontext_s kcontext_t;
typedef struct kexec_s kexec_t; // To use with context structure

typedef enum {
	KCONTEXT_TYPE_NONE,
	// Context for plugin initialization
	KCONTEXT_TYPE_PLUGIN_INIT,
	// Context for plugin finalization
	KCONTEXT_TYPE_PLUGIN_FINI,
	// Context for command's action
	KCONTEXT_TYPE_ACTION,
	// Context for service actions like PTYPE, COND, etc.
	KCONTEXT_TYPE_SERVICE_ACTION,
	KCONTEXT_TYPE_MAX,
} kcontext_type_e;


C_DECL_BEGIN

kcontext_t *kcontext_new(kcontext_type_e type);
void kcontext_free(kcontext_t *context);
kcontext_type_e kcontext_type(const kcontext_t *context);
bool_t kcontext_set_type(kcontext_t *context, kcontext_type_e type);

C_DECL_END

#endif // _klish_kcontext_base_h
