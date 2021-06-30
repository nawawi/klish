/** @file kustore.h
 *
 * @brief Klish user data store. List of kudata_t object.
 */

#ifndef _klish_kustore_h
#define _klish_kustore_h

#include <klish/kudata.h>


typedef struct kustore_s kustore_t;

C_DECL_BEGIN

kustore_t *kustore_new();
void kustore_free(kustore_t *ustore);

bool_t kustore_add_udatas(kustore_t *ustore, kudata_t *udata);
kudata_t *kustore_find_udata(const kustore_t *ustore, const char *name);

// Helper functions for easy access to udata entries
kudata_t *kustore_slot_new(kustore_t *ustore,
	const char *name, void *data, kudata_data_free_fn free_fn);
void *kustore_slot_data(kustore_t *ustore, const char *udata_name);

C_DECL_END

#endif // _klish_kustore_h
