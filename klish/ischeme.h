/** @file ischeme.h
 *
 * @brief Klish Scheme
 */

#ifndef _klish_ischeme_h
#define _klish_ischeme_h

#include <faux/error.h>
#include <klish/iptype.h>
#include <klish/iplugin.h>
#include <klish/iview.h>


#define VIEW_LIST .views = &(iview_t * []) {
#define END_VIEW_LIST NULL }
#define VIEW &(iview_t)

#define PTYPE_LIST .ptypes = &(iptype_t * []) {
#define END_PTYPE_LIST NULL }
#define PTYPE &(iptype_t)

#define COMMAND_LIST .commands = &(icommand_t * []) {
#define END_COMMAND_LIST NULL }
#define COMMAND &(icommand_t)

#define PARAM_LIST .params = &(iparam_t * []) {
#define END_PARAM_LIST NULL }
#define PARAM &(iparam_t)

#define ACTION_LIST .actions = &(iaction_t * []) {
#define END_ACTION_LIST NULL }
#define ACTION &(iaction_t)

#define PLUGIN_LIST .plugins = &(iplugin_t * []) {
#define END_PLUGIN_LIST NULL }
#define PLUGIN &(iplugin_t)


typedef struct ischeme_s {
	char *name;
	iplugin_t * (*plugins)[];
	iptype_t * (*ptypes)[];
	iview_t * (*views)[];
} ischeme_t;

C_DECL_BEGIN

bool_t ischeme_parse_nested(const ischeme_t *ischeme, kscheme_t *kscheme,
	faux_error_t *error);
bool_t ischeme_load(const ischeme_t *ischeme, kscheme_t *kscheme,
	faux_error_t *error);
char *ischeme_deploy(const kscheme_t *scheme, int level);

C_DECL_END

#endif // _klish_ischeme_h
