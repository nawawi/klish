/** @file kcontext_base.h
 *
 * @brief Klish base context to pass to plugin's functions
 */

#ifndef _klish_kcontext_base_h
#define _klish_kcontext_base_h

#include <faux/faux.h>


typedef struct kcontext_s kcontext_t;

typedef enum {
	KCONTEXT_NONE,
	KCONTEXT_PLUGIN_INIT,
	KCONTEXT_PLUGIN_FINI,
	KCONTEXT_PLUGIN_ACTION
} kcontext_type_e;


C_DECL_BEGIN

kcontext_t *kcontext_new(kcontext_type_e type);
void kcontext_free(kcontext_t *context);
kcontext_type_e kcontext_type(const kcontext_t *context);
bool_t kcontext_set_type(kcontext_t *context, kcontext_type_e type);

C_DECL_END

#endif // _klish_kcontext_base_h
