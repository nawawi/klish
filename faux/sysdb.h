/** @file sysdb.h
 * @brief Public interface for faux system database (passwd, group etc) functions.
 */

#ifndef _faux_sysdb_h
#define _faux_sysdb_h

#include <stddef.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#ifdef HAVE_GRP_H
#include <grp.h>
#endif

#include "faux/faux.h"

C_DECL_BEGIN

// Wrappers for ugly getpwnam_r()-like functions
#ifdef HAVE_PWD_H
struct passwd *faux_sysdb_getpwnam(const char *name);
struct passwd *faux_sysdb_getpwuid(uid_t uid);
#endif
#ifdef HAVE_GRP_H
struct group *faux_sysdb_getgrnam(const char *name);
struct group *faux_sysdb_getgrgid(gid_t gid);
#endif

C_DECL_END

#endif
