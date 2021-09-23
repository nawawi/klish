/*
 * private.h
 */

#ifndef _plugins_klish_h
#define _plugins_klish_h

#include <faux/faux.h>
#include <klish/kcontext_base.h>


C_DECL_BEGIN

int klish_nop(kcontext_t *context);
int klish_tsym(kcontext_t *context);

// PTYPEs
int klish_ptype_COMMAND(kcontext_t *context);


C_DECL_END


#endif
