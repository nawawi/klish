/** @file kscheme.h
 *
 * @brief Klish Scheme
 */

#ifndef _klish_kscheme_h
#define _klish_kscheme_h


#include <klish/kptype.h>
#include <klish/kaction.h>
#include <klish/kparam.h>
#include <klish/kcommand.h>
#include <klish/kview.h>


#define VIEW_LIST .views = &(iview_t * []) {
#define END_VIEW_LIST NULL }
#define VIEW &(iview_t)

#define COMMAND_LIST .commands = &(icommand_t * []) {
#define END_COMMAND_LIST NULL }
#define COMMAND &(icommand_t)


typedef struct kscheme_s kscheme_t;

typedef struct ischeme_s {
	char *name;
	iptype_t * (*ptypes)[];
	iview_t * (*views)[];
} ischeme_t;


C_DECL_BEGIN

kscheme_t *kscheme_new(void);
void kscheme_free(kscheme_t *scheme);
bool_t kscheme_add_view(kscheme_t *scheme, kview_t *view);
kview_t *kscheme_find_view(const kscheme_t *scheme, const char *name);

C_DECL_END

#endif // _klish_kscheme_h
