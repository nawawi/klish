/** @file kudata.h
 *
 * @brief Klish user data
 */

#ifndef _klish_kudata_h
#define _klish_kudata_h

typedef struct kudata_s kudata_t;

typedef bool_t (*kudata_data_free_fn)(void *data);

C_DECL_BEGIN

// kudata_t
kudata_t *kudata_new(const char *name);
void kudata_free(kudata_t *udata);

const char *kudata_name(const kudata_t *udata);
void *kudata_data(const kudata_t *udata);
bool_t kudata_set_data(kudata_t *udata, void *data);
kudata_data_free_fn kudata_free_fn(const kudata_t *udata);
bool_t kudata_set_free_fn(kudata_t *udata, kudata_data_free_fn free_fn);

C_DECL_END

#endif // _klish_kudata_h
