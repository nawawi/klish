/** @file ischeme.h
 *
 * @brief Klish Scheme
 */

#ifndef _klish_ischeme_h
#define _klish_ischeme_h

#include <faux/error.h>
#include <klish/iplugin.h>
#include <klish/ientry.h>
#include <klish/kscheme.h>

#define ACTION_LIST .actions = &(iaction_t * []) {
#define END_ACTION_LIST NULL }
#define ACTION &(iaction_t)

#define PLUGIN_LIST .plugins = &(iplugin_t * []) {
#define END_PLUGIN_LIST NULL }
#define PLUGIN &(iplugin_t)

#define ENTRY_LIST .entrys = &(ientry_t * []) {
#define END_ENTRY_LIST NULL }
#define ENTRY &(ientry_t)


typedef struct ischeme_s {
	char *name;
	ientry_t * (*entrys)[];
	iplugin_t * (*plugins)[];
} ischeme_t;

C_DECL_BEGIN

bool_t ischeme_parse_nested(const ischeme_t *ischeme, kscheme_t *kscheme,
	faux_error_t *error);
bool_t ischeme_load(const ischeme_t *ischeme, kscheme_t *kscheme,
	faux_error_t *error);
char *ischeme_deploy(const kscheme_t *scheme, int level);

C_DECL_END

#endif // _klish_ischeme_h
